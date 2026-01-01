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
        private readonly MidiService _midiService;

        public IEnumerable<string> InputDevices => _midiService.GetInputDevices();
        public IEnumerable<string> OutputDevices => _midiService.GetOutputDevices();
        public List<int> MidiChannels { get; } = Enumerable.Range(1, 16).ToList();
        public List<string> Themes { get; } = new List<string> { "Light", "Dark", "System" };

        [ObservableProperty]
        private string _selectedInputDevice;

        [ObservableProperty]
        private string _selectedOutputDevice;

        [ObservableProperty]
        private int _selectedInputChannel;

        [ObservableProperty]
        private int _selectedOutputChannel;
        
        [ObservableProperty]
        private string _selectedTheme;

        public ConfigurationViewModel(MidiService midiService)
        {
            _midiService = midiService;

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

            _midiService.SelectDevices(SelectedInputDevice, SelectedOutputDevice);
            
            if (App.Current != null)
            {
                App.Current.UserAppTheme = SelectedTheme switch
                {
                    "Light" => AppTheme.Light,
                    "Dark" => AppTheme.Dark,
                    _ => AppTheme.Unspecified
                };
            }
        }
    }
}
