using Antmicro.Renode.Peripherals.SPI;

namespace Antmicro.Renode.Peripherals.Mocks
{
    // Register-level SPI stub for startup; the motion registers remain zero.
    public class LisMPaw3222 : ISPIPeripheral
    {
        public void Reset()
        {
            System.Array.Clear(registers, 0, registers.Length);
            pending = false;
        }

        public byte Transmit(byte value)
        {
            if (!pending)
            {
                command = value;
                pending = true;
                return 0;
            }
            pending = false;
            int address = command & 0x7f;
            if ((command & 0x80) != 0)
            {
                registers[address] = value;
                return 0;
            }
            return address == 0 ? (byte)0x30 : registers[address];
        }

        public void FinishTransmission() { }
        private readonly byte[] registers = new byte[128];
        private byte command;
        private bool pending;
    }
}
