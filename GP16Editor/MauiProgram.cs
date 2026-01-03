using CommunityToolkit.Maui;
using GP16Editor.Views;
using Microsoft.Extensions.Logging;
using GP16Editor.Core;
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
            .UseMauiCommunityToolkit()
			.ConfigureFonts(fonts =>
			{
				fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
				fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
			});

		builder.Logging.AddDebug();
		System.Diagnostics.Trace.Listeners.Add(new System.Diagnostics.ConsoleTraceListener());

        // Register SysExService as a singleton
        builder.Services.AddSingleton<SysExService>();
        // Register MidiService, injecting SysExService
        builder.Services.AddSingleton<MidiService>(provider => 
            new MidiService(
                provider.GetRequiredService<SysExService>()
            ));
        builder.Services.AddSingleton<PatchService>();
        builder.Services.AddTransient<ConfigurationView>();
        builder.Services.AddTransient<ConfigurationViewModel>();
        builder.Services.AddSingleton<MainPage>();
        builder.Services.AddSingleton<MainViewModel>(provider =>
            new MainViewModel(
                provider.GetRequiredService<MidiService>(),
                provider.GetRequiredService<PatchService>(),
                provider
            ));
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
