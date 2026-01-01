using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Melanchall.DryWetMidi.Core;
using Melanchall.DryWetMidi.Multimedia;

namespace GP16Editor.Core
{
    public class MidiService : IDisposable
    {
        private IInputDevice? _inputDevice;
        private IOutputDevice? _outputDevice;
        private bool _disposed = false;
        private readonly SysExService _sysExService;
        public int InputChannel { get; set; } = 1;
        public int OutputChannel { get; set; } = 1;
        public byte DeviceId { get; set; } = 0;

        public event EventHandler<NormalSysExEvent>? SysExReceived;
        public event EventHandler<string>? ErrorOccurred;

        public MidiService(SysExService sysExService)
        {
            _sysExService = sysExService;
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
                try
                {
                    _inputDevice = inputDevice;
                    _outputDevice = outputDevice;

                    _inputDevice.EventReceived += OnEventReceived;
                    _inputDevice.StartEventsListening();
                    DeviceId = (byte)(OutputChannel - 1);
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error starting MIDI devices: {ex.Message}");

                    if (_inputDevice != null)
                    {
                        _inputDevice.EventReceived -= OnEventReceived;
                        _inputDevice.Dispose();
                        _inputDevice = null;
                    }

                    if (_outputDevice != null)
                    {
                        _outputDevice.Dispose();
                        _outputDevice = null;
                    }

                    ErrorOccurred?.Invoke(this, $"Could not start MIDI devices: {ex.Message}");
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"MIDI devices not available: Input='{inputDeviceName}' ({inputDevice != null}), Output='{outputDeviceName}' ({outputDevice != null})");
                ErrorOccurred?.Invoke(this, "Selected MIDI devices are not available.");
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

        public async Task SendSysExAsync(ICollection<byte> data)
        {
            if (_outputDevice == null)
            {
                System.Diagnostics.Debug.WriteLine("MIDI Output device not selected.");
                return;
            }

            // The NormalSysExEvent constructor expects the data bytes *without* F0 and F7.
            // My BuildDt1Message and BuildRq1Message methods return the full SysEx message
            // including F0 and F7. So I need to strip them for the DryWetMidi library.
            var sysExDataWithoutF0F7 = data.Skip(1).Take(data.Count - 2).ToArray();
            var sysExEvent = new NormalSysExEvent(sysExDataWithoutF0F7);
            
            _outputDevice.SendEvent(sysExEvent);
            // Implement the required delay for Roland devices between large messages
            await Task.Delay(50); // 50ms delay as specified in GEMINI.md
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

        public Task RequestDataDump(byte[] address, byte[] size)
        {
            var sysexMessage = _sysExService.BuildRq1Message(DeviceId, address, size);
            return SendSysExAsync(sysexMessage.ToList());
        }

        public Task SendParameterChange(byte[] address, byte value)
        {
            var sysexMessage = _sysExService.BuildDt1Message(DeviceId, address, new[] { value });
            return SendSysExAsync(sysexMessage.ToList());
        }
    }
}
