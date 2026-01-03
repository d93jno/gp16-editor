using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using GP16Editor.Core;
using Microsoft.Maui.Storage;

namespace GP16Editor.ViewModels
{
    public partial class ConfigurationViewModel : ObservableObject
    {
        private IEnumerable<string> _inputDevices;
        public IEnumerable<string> InputDevices
        {
            get => _inputDevices;
            set
            {
                if (SetProperty(ref _inputDevices, value))
                {
                    NoInputDevicesFound = !_inputDevices.Any();
                }
            }
        }

        private IEnumerable<string> _outputDevices;
        public IEnumerable<string> OutputDevices
        {
            get => _outputDevices;
            set
            {
                if (SetProperty(ref _outputDevices, value))
                {
                    NoOutputDevicesFound = !_outputDevices.Any();
                }
            }
        }

        private bool _noInputDevicesFound;
        public bool NoInputDevicesFound
        {
            get => _noInputDevicesFound;
            set => SetProperty(ref _noInputDevicesFound, value);
        }

        private bool _noOutputDevicesFound;
        public bool NoOutputDevicesFound
        {
            get => _noOutputDevicesFound;
            set => SetProperty(ref _noOutputDevicesFound, value);
        }

        public List<int> MidiChannels { get; } = [.. Enumerable.Range(1, 16)];
        public List<string> Themes { get; } = ["Light", "Dark", "System"];

        private string _selectedInputDevice;
        public string SelectedInputDevice
        {
            get => _selectedInputDevice;
            set => SetProperty(ref _selectedInputDevice, value);
        }

        private string _selectedOutputDevice;
        public string SelectedOutputDevice
        {
            get => _selectedOutputDevice;
            set => SetProperty(ref _selectedOutputDevice, value);
        }

        private int _selectedInputChannel;
        public int SelectedInputChannel
        {
            get => _selectedInputChannel;
            set => SetProperty(ref _selectedInputChannel, value);
        }

        private int _selectedOutputChannel;
        public int SelectedOutputChannel
        {
            get => _selectedOutputChannel;
            set => SetProperty(ref _selectedOutputChannel, value);
        }
        
        private string _selectedTheme;
        public string SelectedTheme
        {
            get => _selectedTheme;
            set => SetProperty(ref _selectedTheme, value);
        }

        public ConfigurationViewModel()
        {
            _inputDevices = [];
            _outputDevices = [];
            
            _selectedInputDevice = Preferences.Get("SelectedInputDevice", string.Empty);
            _selectedOutputDevice = Preferences.Get("SelectedOutputDevice", string.Empty);
            _selectedInputChannel = Preferences.Get("SelectedInputChannel", 1);
            _selectedOutputChannel = Preferences.Get("SelectedOutputChannel", 1);
            _selectedTheme = Preferences.Get("SelectedTheme", "System");
        }

        [RelayCommand]
        private void Save()
        {
            Preferences.Set("SelectedInputDevice", SelectedInputDevice);
            Preferences.Set("SelectedOutputDevice", SelectedOutputDevice);
            Preferences.Set("SelectedInputChannel", SelectedInputChannel);
            Preferences.Set("SelectedOutputChannel", SelectedOutputChannel);
            Preferences.Set("SelectedTheme", SelectedTheme);

            ThemeManager.SetTheme(SelectedTheme);
        }
    }
}
