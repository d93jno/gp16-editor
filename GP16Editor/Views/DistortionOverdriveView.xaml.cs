using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class DistortionOverdriveView : ContentView
{
	public DistortionOverdriveView()
	{
		InitializeComponent();
		BindingContext = new DistortionOverdriveViewModel();
	}
}