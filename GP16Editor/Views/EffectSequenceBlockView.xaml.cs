using GP16Editor.Models;
using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class EffectSequenceBlockView : ContentView
{
    private EffectSequenceBlockViewModel? _viewModel;

    public EffectSequenceBlockView()
    {
        InitializeComponent();

        // For demo, initialize with Block A effects
        var effects = new List<EffectSequenceItem>
        {
            new() { Id = 1, Name = "Compressor", Icon = "🎛️", IsEnabled = true, Order = 0 },
            new() { Id = 2, Name = "Distortion", Icon = "🔊", IsEnabled = true, Order = 1 },
            new() { Id = 3, Name = "Picking Filter", Icon = "🎸", IsEnabled = true, Order = 2 },
            new() { Id = 4, Name = "Step Phaser", Icon = "🌊", IsEnabled = true, Order = 3 },
            new() { Id = 5, Name = "Parametric EQ", Icon = "📊", IsEnabled = true, Order = 4 },
            new() { Id = 6, Name = "Noise Suppressor", Icon = "🔇", IsEnabled = true, Order = 5 }
        };

        _viewModel = new EffectSequenceBlockViewModel("Block A", effects);
        BindingContext = _viewModel;

        // Update orders when collection changes (e.g., reordering)
        _viewModel.Effects.CollectionChanged += (s, e) =>
        {
            for (int i = 0; i < _viewModel.Effects.Count; i++)
            {
                _viewModel.Effects[i].Order = i;
            }
        };
    }
}