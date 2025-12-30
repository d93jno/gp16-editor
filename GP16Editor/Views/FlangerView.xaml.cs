using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class FlangerView : ContentView
{
	public FlangerView()
	{
		InitializeComponent();
		BindingContext = new FlangerViewModel();
	}
}