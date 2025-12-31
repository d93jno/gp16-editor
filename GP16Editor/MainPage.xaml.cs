using GP16Editor.ViewModels;

namespace GP16Editor;

public partial class MainPage : ContentPage
{
    private bool _initialized = false;
	public MainPage(MainViewModel viewModel)
	{
		InitializeComponent();
		BindingContext = viewModel;
	}

    protected override async void OnAppearing()
    {
        base.OnAppearing();
        if (!_initialized)
        {
            _initialized = true;
            if (BindingContext is MainViewModel vm)
            {
                await vm.InitializeAsync();
            }
        }
    }
}
