using GP16Editor.ViewModels;

namespace GP16Editor.Views;

public partial class CompressorView : ContentView
{
	public CompressorView()
	{
		InitializeComponent();
		BindingContext = new CompressorViewModel();
	}
}