using GP16Editor.Services;
using GP16Editor.Models;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Input;
using Microsoft.Maui.Controls;
using Microsoft.Maui.Storage;

namespace GP16Editor.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly MidiService _midiService;
        private Patch _currentPatch;
        public ICommand RequestPatchCommand { get; }

        private const string SelectedInputDeviceKey = "SelectedInputDevice";
        private const string SelectedOutputDeviceKey = "SelectedOutputDevice";

        public Patch CurrentPatch
        {
            get => _currentPatch;
            set
            {
                if (_currentPatch != null)
                {
                    _currentPatch.PropertyChanged -= Patch_PropertyChanged;
                }
                _currentPatch = value;
                if (_currentPatch != null)
                {
                    _currentPatch.PropertyChanged += Patch_PropertyChanged;
                }
                OnPropertyChanged(nameof(CurrentPatch));
            }
        }

        public EffectSequenceBlockViewModel BlockAViewModel { get; }
        public EffectSequenceBlockViewModel BlockBViewModel { get; }

        public MainViewModel(MidiService midiService)
        {
            _midiService = midiService;
            _currentPatch = new Patch();
            CurrentPatch = _currentPatch;
            InputDevices = new ObservableCollection<string>(_midiService.GetInputDevices());
            OutputDevices = new ObservableCollection<string>(_midiService.GetOutputDevices());

            // Load saved device selections
            string? savedInput = Preferences.Get(SelectedInputDeviceKey, null);
            if (savedInput != null && InputDevices.Contains(savedInput))
            {
                SelectedInputDevice = savedInput;
            }
            else if (savedInput != null)
            {
                Preferences.Remove(SelectedInputDeviceKey);
            }

            string? savedOutput = Preferences.Get(SelectedOutputDeviceKey, null);
            if (savedOutput != null && OutputDevices.Contains(savedOutput))
            {
                SelectedOutputDevice = savedOutput;
            }
            else if (savedOutput != null)
            {
                Preferences.Remove(SelectedOutputDeviceKey);
            }

            RequestPatchCommand = new Command(RequestPatch);

            // Initialize Block A with demo effects
            var blockAEffects = new List<EffectSequenceItem>
            {
                new() { Id = 1, Name = "Compressor", Icon = "🔊", IsEnabled = true },
                new() { Id = 2, Name = "Distortion/Overdrive", Icon = "🎸", IsEnabled = true },
                new() { Id = 3, Name = "Picking Filter", Icon = "🎶", IsEnabled = true },
                new() { Id = 4, Name = "Step Phaser", Icon = "🌊", IsEnabled = true },
                new() { Id = 5, Name = "Parametric EQ", Icon = "🎛️", IsEnabled = true },
                new() { Id = 6, Name = "Noise Suppressor", Icon = "🔇", IsEnabled = true }
            };
            BlockAViewModel = new EffectSequenceBlockViewModel("Block A", blockAEffects);

            // Initialize Block B with demo effects
            var blockBEffects = new List<EffectSequenceItem>
            {
                new() { Id = 1, Name = "Short Delay", Icon = "⏱️", IsEnabled = true },
                new() { Id = 2, Name = "Chorus", Icon = "🌊", IsEnabled = true },
                new() { Id = 3, Name = "Auto Panpot", Icon = "🔄", IsEnabled = true },
                new() { Id = 4, Name = "Tap Delay", Icon = "🎼", IsEnabled = true },
                new() { Id = 5, Name = "Reverb", Icon = "🏞️", IsEnabled = true },
                new() { Id = 6, Name = "Lineout Filter", Icon = "🔊", IsEnabled = true }
            };
            BlockBViewModel = new EffectSequenceBlockViewModel("Block B", blockBEffects);
        }

        private void Patch_PropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(Patch.Sustain))
            {
                // Temporary address for the compressor's sustain parameter.
                // This should be replaced with the correct value from the GP-16 manual.
                var address = new byte[] { 0x01, 0x00, 0x00, 0x00 };
                _midiService.SendParameterChange(address, (byte)CurrentPatch.Sustain);
            }
        }

        private void RequestPatch()
        {
            // Temporary address and size for a single patch dump.
            // This should be replaced with the correct values from the GP-16 manual.
            var address = new byte[] { 0x00, 0x00, 0x00 };
            var size = new byte[] { 0x00, 0x01, 0x00, 0x00 };
            _midiService.RequestDataDump(address, size);
        }

        public ObservableCollection<string> InputDevices { get; }
        public ObservableCollection<string> OutputDevices { get; }

        private string? _selectedInputDevice;
        public string? SelectedInputDevice
        {
            get => _selectedInputDevice;
            set
            {
                _selectedInputDevice = value;
                Preferences.Set(SelectedInputDeviceKey, value);
                OnPropertyChanged(nameof(SelectedInputDevice));
                CheckAndSelectDevices();
            }
        }

        private string? _selectedOutputDevice;
        public string? SelectedOutputDevice
        {
            get => _selectedOutputDevice;
            set
            {
                _selectedOutputDevice = value;
                Preferences.Set(SelectedOutputDeviceKey, value);
                OnPropertyChanged(nameof(SelectedOutputDevice));
                CheckAndSelectDevices();
            }
        }

        private void CheckAndSelectDevices()
        {
            if (!string.IsNullOrEmpty(SelectedInputDevice) && !string.IsNullOrEmpty(SelectedOutputDevice))
            {
                _midiService.SelectDevices(SelectedInputDevice, SelectedOutputDevice);
            }
        }


        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
