using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class PickingFilterView : ContentView
{
	public PickingFilterView()
	{
		InitializeComponent();
		BindingContext = new PickingFilterViewModel();
	}
}