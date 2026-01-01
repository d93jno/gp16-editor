using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Maui.Views;
using GP16Editor.Models;
using GP16Editor.Services;
using GP16Editor.Views;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Windows.Input;
using Microsoft.Maui.Storage;
using Microsoft.Maui.Controls;

namespace GP16Editor.ViewModels
{
    public partial class MainViewModel : ObservableObject
    {
        private readonly MidiService _midiService;
        private readonly IServiceProvider _serviceProvider;
        private Patch _currentPatch;
        public ICommand RequestPatchCommand { get; }

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

        public MainViewModel(MidiService midiService, IServiceProvider serviceProvider)
        {
            _midiService = midiService;
            _serviceProvider = serviceProvider;
            _currentPatch = new Patch();
            CurrentPatch = _currentPatch;
            
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
        
        [RelayCommand]
        private void ShowSettings()
        {
            var popup = _serviceProvider.GetRequiredService<ConfigurationView>();
            if (Application.Current?.MainPage != null)
            {
                Application.Current.MainPage.ShowPopup(popup);
            }
        }

        private void Patch_PropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(Patch.Compressor.Sustain))
            {
                // Temporary address for the compressor's sustain parameter.
                // This should be replaced with the correct value from the GP-16 manual.
                var address = new byte[] { 0x00, 0x00, 0x06 };
                _midiService.SendParameterChange(address, (byte)CurrentPatch.Compressor.Sustain);
            }
        }

        private void RequestPatch()
        {
            // Temporary address and size for a single patch dump.
            // This should be replaced with the correct values from the GP-16 manual.
            var address = new byte[] { 0x00, 0x00, 0x00 };
            var size = new byte[] { 0x00, 0x00, 0x46 };
            _midiService.RequestDataDump(address, size);
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
