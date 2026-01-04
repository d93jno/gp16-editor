using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Maui.Views;
using GP16Editor.Models;
using GP16Editor.Core;
using GP16Editor.Views;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using Microsoft.Maui.Storage;
using Microsoft.Maui.Controls;

namespace GP16Editor.ViewModels
{
    public partial class MainViewModel : ObservableObject
    {
        private readonly MidiService _midiService;
        private readonly PatchService _patchService;
        private readonly IServiceProvider _serviceProvider;
        private Patch _currentPatch;

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

        private string _searchText = "";
        public string SearchText
        {
            get => _searchText;
            set
            {
                if (SetProperty(ref _searchText, value))
                {
                    FilterPatches();
                }
            }
        }

        public ObservableCollection<PatchListItem> AllPatches { get; } = new();
        public ObservableCollection<PatchListItem> FilteredPatches { get; } = new();

        private PatchListItem? _selectedPatch;
        public PatchListItem? SelectedPatch
        {
            get => _selectedPatch;
            set
            {
                if (SetProperty(ref _selectedPatch, value))
                {
                    // Here you would load the actual patch data
                    CurrentPatch = new Patch { PatchName = value?.Name ?? "New Patch" };
                }
            }
        }

        private bool _isInputDeviceSelected;
        public bool IsInputDeviceSelected
        {
            get => _isInputDeviceSelected;
            set => SetProperty(ref _isInputDeviceSelected, value);
        }

        private bool _isOutputDeviceSelected;
        public bool IsOutputDeviceSelected
        {
            get => _isOutputDeviceSelected;
            set => SetProperty(ref _isOutputDeviceSelected, value);
        }

        public bool CanRefresh => IsInputDeviceSelected && IsOutputDeviceSelected;

        public EffectSequenceBlockViewModel BlockAViewModel { get; }
        public EffectSequenceBlockViewModel BlockBViewModel { get; }

        public CompressorViewModel CompressorViewModel { get; }
        public DistortionOverdriveViewModel DistortionOverdriveViewModel { get; }
        public PickingFilterViewModel PickingFilterViewModel { get; }
        public StepPhaserViewModel StepPhaserViewModel { get; }
        public ParametricEQViewModel ParametricEQViewModel { get; }
        public NoiseSuppressorViewModel NoiseSuppressorViewModel { get; }
        public ShortDelayViewModel ShortDelayViewModel { get; }
        public ChorusViewModel ChorusViewModel { get; }
        public FlangerViewModel FlangerViewModel { get; }
        public PitchShifterViewModel PitchShifterViewModel { get; }
        public SpaceDViewModel SpaceDViewModel { get; }
        public AutoPanpotViewModel AutoPanpotViewModel { get; }
        public TapDelayViewModel TapDelayViewModel { get; }
        public ReverbViewModel ReverbViewModel { get; }
        public LineoutFilterViewModel LineoutFilterViewModel { get; }

        public MainViewModel(MidiService midiService, PatchService patchService, IServiceProvider serviceProvider)
        {
            _midiService = midiService;
            _patchService = patchService;
            _serviceProvider = serviceProvider;
            _currentPatch = new Patch();
            CurrentPatch = _currentPatch;

            // Initialize device selection state from preferences
            var selectedInput = Microsoft.Maui.Storage.Preferences.Get("SelectedInputDevice", string.Empty);
            var selectedOutput = Microsoft.Maui.Storage.Preferences.Get("SelectedOutputDevice", string.Empty);
            IsInputDeviceSelected = !string.IsNullOrEmpty(selectedInput);
            IsOutputDeviceSelected = !string.IsNullOrEmpty(selectedOutput);
            
            InitializePatches();
            FilterPatches();

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
                new() { Id = 3, Name = "Flanger", Icon = "🌊", IsEnabled = false },
                new() { Id = 4, Name = "Pitch Shifter", Icon = "🎵", IsEnabled = false },
                new() { Id = 5, Name = "Space-D", Icon = "🌌", IsEnabled = false },
                new() { Id = 6, Name = "Auto Panpot", Icon = "🔄", IsEnabled = true },
                new() { Id = 7, Name = "Tap Delay", Icon = "🎼", IsEnabled = true },
                new() { Id = 8, Name = "Reverb", Icon = "🏞️", IsEnabled = true },
                new() { Id = 9, Name = "Lineout Filter", Icon = "🔊", IsEnabled = true }
            };
            BlockBViewModel = new EffectSequenceBlockViewModel("Block B", blockBEffects);

            CompressorViewModel = new CompressorViewModel();
            DistortionOverdriveViewModel = new DistortionOverdriveViewModel();
            PickingFilterViewModel = new PickingFilterViewModel();
            StepPhaserViewModel = new StepPhaserViewModel();
            ParametricEQViewModel = new ParametricEQViewModel();
            NoiseSuppressorViewModel = new NoiseSuppressorViewModel();
            ShortDelayViewModel = new ShortDelayViewModel();
            ChorusViewModel = new ChorusViewModel();
            FlangerViewModel = new FlangerViewModel();
            PitchShifterViewModel = new PitchShifterViewModel();
            SpaceDViewModel = new SpaceDViewModel();
            AutoPanpotViewModel = new AutoPanpotViewModel();
            TapDelayViewModel = new TapDelayViewModel();
            ReverbViewModel = new ReverbViewModel();
            LineoutFilterViewModel = new LineoutFilterViewModel();

            // Subscribe to effect changes for visibility updates
            foreach (var effect in BlockAViewModel.Effects)
            {
                effect.PropertyChanged += OnEffectChanged;
            }
            foreach (var effect in BlockBViewModel.Effects)
            {
                effect.PropertyChanged += OnEffectChanged;
            }
        }
        
        private void InitializePatches()
        {
            for (int i = 1; i <= 128; i++)
            {
                var group = (i - 1) < 64 ? "A" : "B";
                var patchInGroup = (i - 1) % 64;
                var bank = patchInGroup / 8 + 1;
                var patchInBank = patchInGroup % 8 + 1;
                AllPatches.Add(new PatchListItem
                {
                    PatchNumber = i,
                    Id = $"{group}{bank}{patchInBank}",
                    Name = $"Patch {i}"
                });
            }
        }

        private void FilterPatches()
        {
            var searchText = SearchText?.ToLower() ?? "";
            var filtered = AllPatches.Where(p => p.DisplayName.ToLower().Contains(searchText));

            FilteredPatches.Clear();
            foreach (var patch in filtered)
            {
                FilteredPatches.Add(patch);
            }
        }

        [RelayCommand]
        private async Task RefreshPatches()
        {
            // Show progress popup
            var progressPopup = new GP16Editor.Views.ProgressPopup();
            int max = 128;
            int current = 0;
            var progress = new Progress<int>(value => {
                current = value;
                progressPopup.SetProgress(current, max);
            });

            // Show the popup (assumes MainPage is the current page)
            var mainPage = Application.Current?.Windows[0].Page;
            var popupTask = mainPage?.ShowPopupAsync(progressPopup);

            var patches = await _patchService.GetAllPatchesAsync(progress);
            AllPatches.Clear();
            foreach (var patch in patches)
            {
                AllPatches.Add(new PatchListItem(patch));
            }
            FilterPatches();

            // Hide the popup
            progressPopup.Close();
        }

        [RelayCommand]
        private async Task ShowSettings()
        {
            var viewModel = _serviceProvider.GetRequiredService<ConfigurationViewModel>();

            var inputDevices = _midiService.GetInputDevices();
            var outputDevices = _midiService.GetOutputDevices();
            System.Diagnostics.Debug.WriteLine($"[DEBUG] Assigning InputDevices to ConfigurationViewModel: {string.Join(", ", inputDevices)}");
            System.Diagnostics.Debug.WriteLine($"[DEBUG] Assigning OutputDevices to ConfigurationViewModel: {string.Join(", ", outputDevices)}");
            viewModel.InputDevices = inputDevices;
            viewModel.OutputDevices = outputDevices;

            var popup = new ConfigurationView(viewModel);
            if (Application.Current?.Windows.Count > 0 && Application.Current.Windows[0].Page != null)
            {
                var result = await Application.Current.Windows[0].Page!.ShowPopupAsync(popup);
                if (result is bool saved && saved)
                {
                    _midiService.SelectDevices(viewModel.SelectedInputDevice, viewModel.SelectedOutputDevice);
                    IsInputDeviceSelected = !string.IsNullOrEmpty(viewModel.SelectedInputDevice);
                    IsOutputDeviceSelected = !string.IsNullOrEmpty(viewModel.SelectedOutputDevice);
                    OnPropertyChanged(nameof(CanRefresh));
                }
            }
        }

        private void Patch_PropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(Patch.Compressor.Sustain))
            {
                // Temporary address for the compressor's sustain parameter.
                // This should be replaced with the correct value from the GP-16 manual.
                var address = new byte[] { 0x00, 0x00, 0x06 };
                System.Diagnostics.Debug.WriteLine($"[DEBUG] Sending parameter change to MIDI device: Address={BitConverter.ToString(address)}, Value={CurrentPatch.Compressor.Sustain}");
                _midiService.SendParameterChange(address, (byte)CurrentPatch.Compressor.Sustain);
            }
        }

        private void OnEffectChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(EffectSequenceItem.IsEnabled) && sender is EffectSequenceItem item)
            {
                switch (item.Name)
                {
                    case "Compressor": OnPropertyChanged(nameof(IsCompressorEnabled)); break;
                    case "Distortion/Overdrive": OnPropertyChanged(nameof(IsDistortionOverdriveEnabled)); break;
                    case "Picking Filter": OnPropertyChanged(nameof(IsPickingFilterEnabled)); break;
                    case "Step Phaser": OnPropertyChanged(nameof(IsStepPhaserEnabled)); break;
                    case "Parametric EQ": OnPropertyChanged(nameof(IsParametricEQEnabled)); break;
                    case "Noise Suppressor": OnPropertyChanged(nameof(IsNoiseSuppressorEnabled)); break;
                    case "Short Delay": OnPropertyChanged(nameof(IsShortDelayEnabled)); break;
                    case "Chorus": OnPropertyChanged(nameof(IsChorusEnabled)); break;
                    case "Flanger": OnPropertyChanged(nameof(IsFlangerEnabled)); break;
                    case "Pitch Shifter": OnPropertyChanged(nameof(IsPitchShifterEnabled)); break;
                    case "Space-D": OnPropertyChanged(nameof(IsSpaceDEnabled)); break;
                    case "Auto Panpot": OnPropertyChanged(nameof(IsAutoPanpotEnabled)); break;
                    case "Tap Delay": OnPropertyChanged(nameof(IsTapDelayEnabled)); break;
                    case "Reverb": OnPropertyChanged(nameof(IsReverbEnabled)); break;
                    case "Lineout Filter": OnPropertyChanged(nameof(IsLineoutFilterEnabled)); break;
                }
            }
        }

        public bool IsCompressorEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Compressor")?.IsEnabled ?? false;
        public bool IsDistortionOverdriveEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Distortion/Overdrive")?.IsEnabled ?? false;
        public bool IsPickingFilterEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Picking Filter")?.IsEnabled ?? false;
        public bool IsStepPhaserEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Step Phaser")?.IsEnabled ?? false;
        public bool IsParametricEQEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Parametric EQ")?.IsEnabled ?? false;
        public bool IsNoiseSuppressorEnabled => BlockAViewModel.Effects.FirstOrDefault(e => e.Name == "Noise Suppressor")?.IsEnabled ?? false;

        public bool IsShortDelayEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Short Delay")?.IsEnabled ?? false;
        public bool IsChorusEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Chorus")?.IsEnabled ?? false;
        public bool IsFlangerEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Flanger")?.IsEnabled ?? false;
        public bool IsPitchShifterEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Pitch Shifter")?.IsEnabled ?? false;
        public bool IsSpaceDEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Space-D")?.IsEnabled ?? false;
        public bool IsAutoPanpotEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Auto Panpot")?.IsEnabled ?? false;
        public bool IsTapDelayEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Tap Delay")?.IsEnabled ?? false;
        public bool IsReverbEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Reverb")?.IsEnabled ?? false;
        public bool IsLineoutFilterEnabled => BlockBViewModel.Effects.FirstOrDefault(e => e.Name == "Lineout Filter")?.IsEnabled ?? false;
    }
}
