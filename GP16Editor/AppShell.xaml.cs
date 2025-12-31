namespace GP16Editor;

public partial class AppShell : Shell
{
    public MainPage TheMainPage { get; }
	public AppShell(MainPage page)
	{
		InitializeComponent();
        TheMainPage = page;
        BindingContext = this;
    }
}
