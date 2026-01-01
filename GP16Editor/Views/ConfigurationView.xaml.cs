using CommunityToolkit.Maui.Views;

namespace GP16Editor.Views;

public partial class ConfigurationView : Popup
{
	public ConfigurationView(ViewModels.ConfigurationViewModel viewModel)
	{
		InitializeComponent();
		BindingContext = viewModel;
	}

    private void OnSaveButtonClicked(object sender, EventArgs e)
    {
        Close();
    }
}
