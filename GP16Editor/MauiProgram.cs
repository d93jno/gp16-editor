using Microsoft.Extensions.Logging;
using GP16Editor.Services;
using GP16Editor.ViewModels;
using SkiaSharp.Views.Maui.Controls.Hosting;
using System.Diagnostics;

namespace GP16Editor;

public static class MauiProgram
{
	public static MauiApp CreateMauiApp()
	{
		var builder = MauiApp.CreateBuilder();
		builder
			.UseMauiApp<App>()
            .UseSkiaSharp()
			.ConfigureFonts(fonts =>
			{
				fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
				fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
			});

		builder.Logging.AddDebug();
		System.Diagnostics.Trace.Listeners.Add(new System.Diagnostics.ConsoleTraceListener());

        builder.Services.AddSingleton<MidiService>();
        builder.Services.AddSingleton<MainPage>();
        builder.Services.AddSingleton<MainViewModel>();
        builder.Services.AddSingleton<AppShell>();

        // Register App with MidiService dependency
        builder.Services.AddSingleton<App>(provider => 
            new App(
                provider.GetRequiredService<MidiService>(),
                provider.GetRequiredService<AppShell>()
            ));

        return builder.Build();
	}
}
