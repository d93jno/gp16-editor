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
        public bool IsConnected => _inputDevice != null && _outputDevice != null;

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
            var devices = InputDevice.GetAll().Select(d => d.Name).ToList();
            System.Diagnostics.Debug.WriteLine($"[DEBUG] Listing Input Devices: {string.Join(", ", devices)}");
            return devices;
        }

        public IEnumerable<string> GetOutputDevices()
        {
            var devices = OutputDevice.GetAll().Select(d => d.Name).ToList();
            System.Diagnostics.Debug.WriteLine($"[DEBUG] Listing Output Devices: {string.Join(", ", devices)}");
            return devices;
        }

        public void SelectDevices(string? inputDeviceName, string? outputDeviceName)
        {
            var inputDevices = InputDevice.GetAll().ToList();
            var outputDevices = OutputDevice.GetAll().ToList();

            var inputDevice = inputDevices.FirstOrDefault(d => d.Name == inputDeviceName);
            var outputDevice = outputDevices.FirstOrDefault(d => d.Name == outputDeviceName);

            // Dispose of all other devices that were not selected
            foreach (var d in inputDevices.Where(d => d != inputDevice)) d.Dispose();
            foreach (var d in outputDevices.Where(d => d != outputDevice)) d.Dispose();

            if (inputDevice != null && outputDevice != null)
            {
                try
                {
                    _inputDevice = inputDevice;
                    _outputDevice = outputDevice;

                    _inputDevice.EventReceived += OnEventReceived;
                    _inputDevice.StartEventsListening();
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[MIDI] Error starting devices: {ex.Message}");

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
                ErrorOccurred?.Invoke(this, "Selected MIDI devices are not available.");
                // Ensure we dispose the ones we did find if the other is missing
                inputDevice?.Dispose();
                outputDevice?.Dispose();
                _inputDevice = null;
                _outputDevice = null;
            }
        }

        private void OnEventReceived(object? sender, MidiEventReceivedEventArgs e)
        {
            if (_disposed)
                return;

            if (e.Event is NormalSysExEvent sysExEvent)
            {
                SysExReceived?.Invoke(this, sysExEvent);
            }
        }

        public async Task SendSysExAsync(ICollection<byte> data)
        {
            if (_outputDevice == null)
            {
                return;
            }

            var sysExDataWithoutF0F7 = data.Skip(1).Take(data.Count - 2).ToArray();
            var sysExEvent = new NormalSysExEvent(sysExDataWithoutF0F7);
            
            _outputDevice.SendEvent(sysExEvent);
            await Task.Delay(50); 
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
                try
                {
                    if (_inputDevice != null)
                    {
                        Console.WriteLine("[MIDI] Closing input device...");
                        _inputDevice.EventReceived -= OnEventReceived;
                        _inputDevice.Dispose();
                        _inputDevice = null;
                    }
                    
                    if (_outputDevice != null)
                    {
                        Console.WriteLine("[MIDI] Closing output device...");
                        _outputDevice.Dispose();
                        _outputDevice = null;
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[MIDI] Error during dispose: {ex.Message}");
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
