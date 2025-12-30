using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class StepPhaserView : ContentView
{
	public StepPhaserView()
	{
		InitializeComponent();
		BindingContext = new StepPhaserViewModel();
	}
}