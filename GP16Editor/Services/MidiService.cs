using Melanchall.DryWetMidi.Core;
using Melanchall.DryWetMidi.Multimedia;

namespace GP16Editor.Services
{
    public class MidiService
    {
        private IInputDevice? _inputDevice;
        private IOutputDevice? _outputDevice;
        private bool _disposed = false;

        public event EventHandler<NormalSysExEvent>? SysExReceived;

        public MidiService()
        {
        }

        ~MidiService()
        {
            Dispose(false);
        }

        public IEnumerable<string> GetInputDevices()
        {
            return InputDevice.GetAll().Select(d => d.Name);
        }

        public IEnumerable<string> GetOutputDevices()
        {
            return OutputDevice.GetAll().Select(d => d.Name);
        }

        public void SelectDevices(string? inputDeviceName, string? outputDeviceName)
        {
            var inputDevices = InputDevice.GetAll();
            var outputDevices = OutputDevice.GetAll();

            var inputDevice = inputDevices.FirstOrDefault(d => d.Name == inputDeviceName);
            var outputDevice = outputDevices.FirstOrDefault(d => d.Name == outputDeviceName);

            System.Diagnostics.Debug.WriteLine($"MIDI devices selected: Input='{inputDeviceName}', Output='{outputDeviceName}'");

            if (inputDevice != null && outputDevice != null)
            {
                _inputDevice = inputDevice;
                _outputDevice = outputDevice;


                _inputDevice.EventReceived += OnEventReceived;
                _inputDevice.StartEventsListening();
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"MIDI devices not available: Input='{inputDeviceName}' ({inputDevice != null}), Output='{outputDeviceName}' ({outputDevice != null})");
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
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_disposed)
                return;

            if (disposing)
            {
                // Dispose managed resources
                try
                {
                    if (_inputDevice != null)
                    {
                        System.Diagnostics.Debug.WriteLine("Stopping MIDI input device event listening");
                        _inputDevice.StopEventsListening();
                        _inputDevice.Dispose();
                        _inputDevice = null;
                    }
                    
                    if (_outputDevice != null)
                    {
                        _outputDevice.Dispose();
                        _outputDevice = null;
                    }
                    
                    System.Diagnostics.Debug.WriteLine("MIDI service disposed successfully");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error disposing MIDI service: {ex.Message}");
                }
            }

            _disposed = true;
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
