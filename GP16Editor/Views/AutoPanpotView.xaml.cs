using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class AutoPanpotView : ContentView
{
	public AutoPanpotView()
	{
		InitializeComponent();
		BindingContext = new AutoPanpotViewModel();
	}
}