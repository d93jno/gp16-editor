using System.Diagnostics;
using System.IO;
using Microsoft.Maui.Storage;
using GP16Editor.Services;

namespace GP16Editor;

public partial class App : Application
{
	private MidiService? _midiService;

	public App(MidiService midiService)
	{
		InitializeComponent();

		_midiService = midiService;

		// Global exception handlers
		AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
		TaskScheduler.UnobservedTaskException += OnUnobservedTaskException;
	}

	private void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
	{
		var exception = e.ExceptionObject as Exception;
		LogException("Unhandled Exception", exception);
		ShowErrorDialog("Unhandled Exception", exception);
	}

    private void OnUnobservedTaskException(object? sender, UnobservedTaskExceptionEventArgs e)
	{
		LogException("Unobserved Task Exception", e.Exception);
		ShowErrorDialog("Unobserved Task Exception", e.Exception);
		e.SetObserved(); // Prevent the exception from crashing the app
	}

	private void LogException(string type, Exception? exception)
	{
		var message = $"{type}: {exception?.Message}\nStack Trace: {exception?.StackTrace}";
		Debug.WriteLine(message);
		// Optionally log to file
		try
		{
			var logPath = Path.Combine(FileSystem.AppDataDirectory, "error.log");
			File.AppendAllText(logPath, $"{DateTime.Now}: {message}\n\n");
		}
		catch
		{
			// Ignore logging errors
		}
	}

	private void ShowErrorDialog(string title, Exception? exception)
	{
		MainThread.BeginInvokeOnMainThread(async () =>
		{
			try
			{
				var message = $"An error occurred:\n\n{exception?.Message}\n\nStack Trace:\n{exception?.StackTrace}";
				var app = Application.Current;
				if (app != null && app.Windows?.Count > 0 && app.Windows[0].Page != null)
				{
					await app.Windows[0].Page.DisplayAlert(title, message, "OK");
				}
			}
			catch
			{
				// If dialog fails, at least we logged it
			}
		});
	}

	protected override Window CreateWindow(IActivationState? activationState)
	{
		return new Window(new AppShell());
	}

	protected override void OnSleep()
	{
		base.OnSleep();
		
		// Clean up MIDI resources when app is suspended/closed
		try
		{
			_midiService?.Dispose();
			_midiService = null;
			Debug.WriteLine("MIDI service disposed during app sleep/close");
		}
		catch (Exception ex)
		{
			Debug.WriteLine($"Error disposing MIDI service: {ex.Message}");
		}
	}
}
