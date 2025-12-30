using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class ReverbView : ContentView
{
	public ReverbView()
	{
		InitializeComponent();
		BindingContext = new ReverbViewModel();
	}
}