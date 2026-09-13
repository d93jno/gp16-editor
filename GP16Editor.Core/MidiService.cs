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
        private readonly object _recordingLock = new();
        private bool _isRecording;
        private List<byte>? _recordedOutgoing;
        private List<byte>? _recordedIncoming;
        public int InputChannel { get; set; } = 1;
        public int OutputChannel { get; set; } = 1;
        public byte DeviceId { get; set; } = 0;
        public bool IsConnected => _inputDevice != null && _outputDevice != null;
        public bool IsInputConnected => _inputDevice != null;
        public bool IsRecording
        {
            get
            {
                lock (_recordingLock)
                {
                    return _isRecording;
                }
            }
        }

        public event EventHandler<NormalSysExEvent>? SysExReceived;
        public event EventHandler<string>? ErrorOccurred;

        public MidiService(SysExService sysExService)
        {
            _sysExService = sysExService;
        }

        public void StartRecording()
        {
            lock (_recordingLock)
            {
                _recordedOutgoing = new List<byte>();
                _recordedIncoming = new List<byte>();
                _isRecording = true;
            }
        }

        public (byte[] Outgoing, byte[] Incoming) StopRecording()
        {
            lock (_recordingLock)
            {
                _isRecording = false;
                var outgoing = _recordedOutgoing?.ToArray() ?? Array.Empty<byte>();
                var incoming = _recordedIncoming?.ToArray() ?? Array.Empty<byte>();
                _recordedOutgoing = null;
                _recordedIncoming = null;
                return (outgoing, incoming);
            }
        }

        private static byte[] ToFullSysExMessage(byte[] data)
        {
            var fullMsg = new List<byte>(data.Length + 2);
            if (data.Length == 0 || data[0] != 0xF0)
                fullMsg.Add(0xF0);
            fullMsg.AddRange(data);
            if (fullMsg[^1] != 0xF7)
                fullMsg.Add(0xF7);
            return fullMsg.ToArray();
        }

        private void RecordOutgoing(ICollection<byte> data)
        {
            lock (_recordingLock)
            {
                if (!_isRecording || _recordedOutgoing == null)
                    return;
                _recordedOutgoing.AddRange(data);
            }
        }

        private void RecordIncoming(byte[] data)
        {
            lock (_recordingLock)
            {
                if (!_isRecording || _recordedIncoming == null)
                    return;
                _recordedIncoming.AddRange(ToFullSysExMessage(data));
            }
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

        public void SelectInputDevice(string? inputDeviceName)
        {
            CloseDevices();

            var inputDevices = InputDevice.GetAll().ToList();
            foreach (var d in OutputDevice.GetAll())
                d.Dispose();

            var inputDevice = inputDevices.FirstOrDefault(d => d.Name == inputDeviceName);
            foreach (var d in inputDevices.Where(d => d != inputDevice))
                d.Dispose();

            if (inputDevice == null)
            {
                ErrorOccurred?.Invoke(this, "Selected MIDI input device is not available.");
                return;
            }

            try
            {
                _inputDevice = inputDevice;
                _inputDevice.EventReceived += OnEventReceived;
                _inputDevice.StartEventsListening();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[MIDI] Error starting input device: {ex.Message}");
                if (_inputDevice != null)
                {
                    _inputDevice.EventReceived -= OnEventReceived;
                    _inputDevice.Dispose();
                    _inputDevice = null;
                }
                ErrorOccurred?.Invoke(this, $"Could not start MIDI input device: {ex.Message}");
            }
        }

        public void SelectDevices(string? inputDeviceName, string? outputDeviceName)
        {
            CloseDevices();

            var inputDevices = InputDevice.GetAll().ToList();
            var outputDevices = OutputDevice.GetAll().ToList();

            var inputDevice = inputDevices.FirstOrDefault(d => d.Name == inputDeviceName);
            var outputDevice = outputDevices.FirstOrDefault(d => d.Name == outputDeviceName);

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
                inputDevice?.Dispose();
                outputDevice?.Dispose();
                _inputDevice = null;
                _outputDevice = null;
            }
        }

        private void CloseDevices()
        {
            var input = _inputDevice;
            var output = _outputDevice;
            _inputDevice = null;
            _outputDevice = null;

            if (input != null)
            {
                try
                {
                    input.EventReceived -= OnEventReceived;
                    input.Dispose();
                }
                catch { }
            }

            if (output != null)
            {
                try
                {
                    output.Dispose();
                }
                catch { }
            }
        }

        private void OnEventReceived(object? sender, MidiEventReceivedEventArgs e)
        {
            if (_disposed)
                return;

            if (e.Event is NormalSysExEvent sysExEvent)
            {
                RecordIncoming(sysExEvent.Data);
                SysExReceived?.Invoke(this, sysExEvent);
            }
        }

        public async Task SendSysExAsync(ICollection<byte> data)
        {
            if (_outputDevice == null)
            {
                return;
            }

            RecordOutgoing(data);

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

            _disposed = true;

            if (disposing)
            {
                var input = _inputDevice;
                var output = _outputDevice;
                _inputDevice = null;
                _outputDevice = null;

                if (input != null)
                    input.EventReceived -= OnEventReceived;

                var disposeTask = Task.Run(() =>
                {
                    try
                    {
                        input?.Dispose();
                    }
                    catch { }

                    try
                    {
                        output?.Dispose();
                    }
                    catch { }
                });

                if (!disposeTask.Wait(TimeSpan.FromSeconds(2)))
                    Console.WriteLine("[MIDI] Device cleanup timed out, continuing shutdown.");
            }
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
