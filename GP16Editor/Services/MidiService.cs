using Melanchall.DryWetMidi.Core;
using Melanchall.DryWetMidi.Multimedia;

namespace GP16Editor.Services
{
    public class MidiService
    {
        private IInputDevice? _inputDevice;
        private IOutputDevice? _outputDevice;

        public event EventHandler<NormalSysExEvent>? SysExReceived;

        public MidiService()
        {
        }

        public IEnumerable<string> GetInputDevices()
        {
            return InputDevice.GetAll().Select(d => d.Name);
        }

        public IEnumerable<string> GetOutputDevices()
        {
            return OutputDevice.GetAll().Select(d => d.Name);
        }

        public void SelectDevices(string inputDeviceName, string outputDeviceName)
        {
            var inputDevices = InputDevice.GetAll();
            var outputDevices = OutputDevice.GetAll();

            var inputDevice = inputDevices.FirstOrDefault(d => d.Name == inputDeviceName);
            var outputDevice = outputDevices.FirstOrDefault(d => d.Name == outputDeviceName);

            if (inputDevice != null && outputDevice != null)
            {
                _inputDevice = inputDevice;
                _outputDevice = outputDevice;

                _inputDevice.EventReceived += OnEventReceived;
                _inputDevice.StartEventsListening();
            }
            else
            {
                // Devices not available, do nothing
                _inputDevice = null;
                _outputDevice = null;
            }
        }

        private void OnEventReceived(object? sender, MidiEventReceivedEventArgs e)
        {
            if (e.Event is NormalSysExEvent sysExEvent)
            {
                var dataBytes = new List<byte> { 0xF0 };
                dataBytes.AddRange(sysExEvent.Data);
                dataBytes.Add(0xF7);
                var hex = string.Join(" ", dataBytes.Select(b => b.ToString("X2")));
                System.Diagnostics.Debug.WriteLine($"MIDI IN: {hex}");

                SysExReceived?.Invoke(this, sysExEvent);
            }
        }

        public void SendSysEx(byte[] data)
        {
            var sysExEvent = new NormalSysExEvent(data);
            _outputDevice?.SendEvent(sysExEvent);
        }

        public void Dispose()
        {
            _inputDevice?.StopEventsListening();
            _inputDevice?.Dispose();
            _outputDevice?.Dispose();
        }

        public byte CalculateChecksum(IEnumerable<byte> addressAndData)
        {
            var sum = addressAndData.Sum(b => b);
            var remainder = sum % 128;
            var checksum = 128 - remainder;
            return (byte)(checksum == 128 ? 0 : checksum);
        }

        public void RequestDataDump(byte[] address, byte[] size)
        {
            var message = new List<byte> { 0xF0, 0x41, 0x10, 0x2A, 0x11 };
            message.AddRange(address);
            message.AddRange(size);
            var checksum = CalculateChecksum(address.Concat(size));
            message.Add(checksum);
            message.Add(0xF7);
            SendSysEx(message.ToArray());
        }

        public void SendParameterChange(byte[] address, byte value)
        {
            var message = new List<byte> { 0xF0, 0x41, 0x10, 0x2A, 0x12 };
            message.AddRange(address);
            message.Add(value);
            var checksum = CalculateChecksum(address.Concat(new[] { value }));
            message.Add(checksum);
            message.Add(0xF7);
            SendSysEx(message.ToArray());
        }
    }
}
