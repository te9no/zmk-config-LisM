// SPDX-License-Identifier: MIT
#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/matrix_transform.h>
#include "modules_profile.h"

#define DEV(label) DEVICE_DT_GET(DT_NODELABEL(label))
enum module_mode { OFF, ENCODER, FOUR_WAY, PAW3222 };
BUILD_ASSERT(ZEROSHIFT_SLOT_COUNT == 1);
ZMK_MATRIX_TRANSFORM_EXTERN(DT_NODELABEL(transform_keymap));

static void key_changed(const struct device *dev, uint32_t row, uint32_t col, bool pressed) {
    int32_t position = zmk_matrix_transform_row_column_to_position(
        ZMK_MATRIX_TRANSFORM_T_FOR_NODE(DT_NODELABEL(transform_keymap)), 4 + row, col);
    if (position >= 0) {
        raise_zmk_position_state_changed((struct zmk_position_state_changed){
            .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL, .position = position,
            .state = pressed, .timestamp = k_uptime_get()});
    }
}

static int start_keys(const struct device *dev) {
    int err = device_init(dev);
    if (err) return err;
    err = kscan_config(dev, key_changed);
    if (err) return err;
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
    if (pm_device_runtime_is_enabled(dev)) return pm_device_runtime_get(dev);
#endif
#if IS_ENABLED(CONFIG_PM_DEVICE)
    return pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
#else
    return kscan_enable_callback(dev);
#endif
}

static void rotated(const struct device *dev, const struct sensor_trigger *trigger) {
    struct sensor_value value;
    if (sensor_sample_fetch(dev) || sensor_channel_get(dev, trigger->chan, &value)) return;
    raise_zmk_sensor_event((struct zmk_sensor_event){
        .sensor_index = 0, .channel_data_size = 1,
        .channel_data = {{.value = value, .channel = trigger->chan}}, .timestamp = k_uptime_get()});
}

bool zeroshift_mode_supported(size_t slot, uint8_t mode) {
    return slot == 0 && mode <= PAW3222;
}

int zeroshift_start_mode(size_t slot, uint8_t mode) {
    static const struct sensor_trigger trigger = {.type = SENSOR_TRIG_DATA_READY,
                                                  .chan = SENSOR_CHAN_ROTATION};
    int err;
    switch (mode) {
    case ENCODER:
        err = device_init(DEV(module_encoder));
        if (err) return err;
        err = start_keys(DEV(encoder_push));
        return err ? err : sensor_trigger_set(DEV(module_encoder), &trigger, rotated);
    case FOUR_WAY:
        return start_keys(DEV(four_way));
    case PAW3222:
        err = device_init(DEV(spi0));
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        return err ? err : device_init(DEV(trackball_central));
#else
        return err ? err : device_init(DEV(trackball_peripheral));
#endif
    default:
        return -EINVAL;
    }
}
