if(CONFIG_ZMK_ZEROSHIFT)
  target_sources(app PRIVATE src/zeroshift/modules.c src/zeroshift/modules_relay.c)
  if(CONFIG_ZMK_ZEROSHIFT_STUDIO)
    generate_inc_file_for_target(app ${CMAKE_CURRENT_SOURCE_DIR}/config/zeroshift.json
                                 ${CMAKE_CURRENT_BINARY_DIR}/descriptor.inc --format=literal)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(app PRIVATE src/zeroshift/modules_rpc.c)
  endif()
endif()


# The pinned split central uses zero as both a valid input ID and an unset marker.
if(CONFIG_ZMK_ZEROSHIFT AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL AND CONFIG_ZMK_INPUT_SPLIT AND CONFIG_ZMK_SPLIT_BLE)
  function(patch_split_input_discovery)
    set(dir "${APPLICATION_SOURCE_DIR}/src/split/bluetooth")
    set(original "${dir}/central.c")
    set(patched "${CMAKE_CURRENT_BINARY_DIR}/split_central_fixed.c")
    get_target_property(sources app SOURCES)
    list(FIND sources "${original}" index)
    if(index EQUAL -1)
      message(FATAL_ERROR "Split input source not found")
    endif()
    file(READ "${original}" code)
    macro(replace_split_fragment before after)
      string(FIND "${code}" "${before}" pos)
      if(pos EQUAL -1)
        message(FATAL_ERROR "Review split input ID patch against pinned ZMK")
      endif()
      string(REPLACE "${before}" "${after}" code "${code}")
    endmacro()
    replace_split_fragment("uint8_t reg;\n};" "uint8_t reg;\n    bool reg_known;\n};")
    replace_split_fragment("!peripheral_input_slots[i].reg);" "!peripheral_input_slots[i].reg_known);")
    replace_split_fragment("peripheral_input_slots[i].reg = 0;" "peripheral_input_slots[i].reg_known = false;")
    replace_split_fragment("input_slot->reg = cpf->description;" "input_slot->reg = cpf->description;\n                input_slot->reg_known = true;")
    replace_split_fragment("zmk_input_split_peripheral_disconnected(peripheral_input_slots[i].reg);"
                           "if (peripheral_input_slots[i].reg_known) {\n                zmk_input_split_peripheral_disconnected(peripheral_input_slots[i].reg);\n            }")
    # One ATT reply can hold several attributes. When switching queries, stop
    # parsing the old reply before its next attribute reaches the new handler.
    replace_split_fragment("switch (params->type) {"
                           "const uint8_t original_type = params->type;\n    const struct bt_uuid *original_uuid = params->uuid;\n    switch (params->type) {")
    replace_split_fragment("    bool subscribed = slot->run_behavior_handle"
[=[    if (params->type != original_type || params->uuid != original_uuid) {
        int err = bt_gatt_discover(conn, params);
        if (err) {
            LOG_ERR("Failed to continue split discovery: %d", err);
        }
        return BT_GATT_ITER_STOP;
    }

    bool subscribed = slot->run_behavior_handle]=])
    file(WRITE "${patched}" "${code}")
    list(REMOVE_AT sources ${index})
    list(APPEND sources "${patched}")
    set_property(TARGET app PROPERTY SOURCES "${sources}")
    set_source_files_properties("${patched}" PROPERTIES INCLUDE_DIRECTORIES "${dir}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
  endfunction()
  cmake_language(DEFER DIRECTORY "${APPLICATION_SOURCE_DIR}" CALL patch_split_input_discovery)
endif()

if(SNIPPET MATCHES "lism-sim-right")
  target_sources(app PRIVATE tests/renode/uart_endpoint.c)
endif()

# Let the CDC workqueue drain its FIFO before Studio retries a short write.
if(CONFIG_ZMK_STUDIO_TRANSPORT_UART AND CONFIG_UART_INTERRUPT_DRIVEN AND CONFIG_ZMK_ZEROSHIFT)
  function(lism_patch_studio_uart)
    get_target_property(sources app SOURCES)
    set(original "${APPLICATION_SOURCE_DIR}/src/studio/uart_rpc_transport.c")
    list(FIND sources "${original}" index)
    if(index EQUAL -1)
      message(FATAL_ERROR "LisM Studio UART source not found")
    endif()
    file(READ "${original}" code)
    foreach(anchor "if (claim_len == 0) {\n                continue;" "ring_buf_get_finish(tx_buf, MAX(sent, 0));")
      string(FIND "${code}" "${anchor}" pos)
      if(pos EQUAL -1)
        message(FATAL_ERROR "Review LisM Studio UART patch against the pinned ZMK source")
      endif()
    endforeach()
    string(REPLACE "if (claim_len == 0) {\n                continue;"
                   "if (claim_len == 0) {\n                break;" code "${code}")
    string(REPLACE "ring_buf_get_finish(tx_buf, MAX(sent, 0));"
                   "ring_buf_get_finish(tx_buf, MAX(sent, 0));\n            if (sent < (int)claim_len) {\n                break;\n            }" code "${code}")
    set(patched "${CMAKE_CURRENT_BINARY_DIR}/lism_studio_uart.c")
    file(WRITE "${patched}" "${code}")
    list(REMOVE_AT sources ${index})
    list(APPEND sources "${patched}")
    set_property(TARGET app PROPERTY SOURCES "${sources}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
  endfunction()
  cmake_language(DEFER DIRECTORY "${APPLICATION_SOURCE_DIR}" CALL lism_patch_studio_uart)
endif()
