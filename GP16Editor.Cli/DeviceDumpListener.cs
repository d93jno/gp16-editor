using GP16Editor.Core;
using Melanchall.DryWetMidi.Core;

namespace GP16Editor.Cli;

internal static class DeviceDumpListener
{
    private static readonly TimeSpan IdleTimeout = TimeSpan.FromMilliseconds(2000);

    public static async Task RunAsync(string[] args)
    {
        Console.WriteLine("GP-16 Device Dump Listener");
        Console.WriteLine("--------------------------");
        Console.WriteLine("Passive capture: start a bulk dump on the GP-16 front panel.");
        Console.WriteLine("Recording stops after 2s of silence once data has arrived, or when you press Enter.");
        Console.WriteLine();

        var sysExService = new SysExService();
        using var midiService = new MidiService(sysExService);

        var inputDevices = midiService.GetInputDevices().ToList();
        if (inputDevices.Count == 0)
        {
            Console.WriteLine("No MIDI input devices found.");
            return;
        }

        Console.WriteLine("Available MIDI Input Devices:");
        for (var i = 0; i < inputDevices.Count; i++)
            Console.WriteLine($"  {i}: {inputDevices[i]}");

        var defaultInputIndex = inputDevices.FindIndex(d => d.Contains("MIDI", StringComparison.OrdinalIgnoreCase));
        if (defaultInputIndex < 0)
            defaultInputIndex = 0;

        Console.Write($"Select input device (number) [default {defaultInputIndex}]: ");
        var inputDeviceIndex = defaultInputIndex;
        while (true)
        {
            var line = Console.ReadLine();
            if (string.IsNullOrWhiteSpace(line))
                break;
            if (int.TryParse(line, out inputDeviceIndex) && inputDeviceIndex >= 0 && inputDeviceIndex < inputDevices.Count)
                break;
            Console.Write("Invalid selection. Please select input device (number): ");
        }

        var inputName = inputDevices[inputDeviceIndex];
        string? midiError = null;
        void OnError(object? _, string error) => midiError = error;
        midiService.ErrorOccurred += OnError;
        midiService.SelectInputDevice(inputName);
        midiService.ErrorOccurred -= OnError;

        if (!midiService.IsInputConnected)
        {
            Console.WriteLine(midiError ?? "Failed to open MIDI input.");
            return;
        }

        var outputPath = ResolveOutputPath(args);
        var messageCount = 0;
        var byteCount = 0;
        var lastMessageAt = DateTime.UtcNow;
        var receivedAny = false;
        var stopRequested = false;

        void OnSysEx(object? _, NormalSysExEvent e)
        {
            receivedAny = true;
            lastMessageAt = DateTime.UtcNow;
            Interlocked.Increment(ref messageCount);
            Interlocked.Add(ref byteCount, e.Data?.Length ?? 0);
            Console.Write($"\r  Messages: {messageCount}  raw event bytes ~{byteCount}   ");
        }

        midiService.StartRecording();
        midiService.SysExReceived += OnSysEx;

        Console.WriteLine();
        Console.WriteLine($"Listening on '{inputName}'.");
        Console.WriteLine("Trigger FULL / ALL bulk dump on the GP-16 now.");
        Console.WriteLine("Press Enter to stop early.");
        Console.WriteLine();

        try
        {
            while (!stopRequested)
            {
                while (Console.KeyAvailable)
                {
                    var key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.Enter)
                        stopRequested = true;
                }

                if (receivedAny && DateTime.UtcNow - lastMessageAt >= IdleTimeout)
                    break;

                await Task.Delay(50);
            }
        }
        finally
        {
            midiService.SysExReceived -= OnSysEx;
            var (_, incoming) = midiService.StopRecording();
            var fullPath = Path.GetFullPath(outputPath);
            var directory = Path.GetDirectoryName(fullPath);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);
            await File.WriteAllBytesAsync(fullPath, incoming);
            Console.WriteLine();
            Console.WriteLine($"Wrote {incoming.Length} bytes ({messageCount} SysEx events) to:");
            Console.WriteLine($"  {fullPath}");
        }
    }

    private static string ResolveOutputPath(string[] args)
    {
        for (var i = 0; i < args.Length - 1; i++)
        {
            if (string.Equals(args[i], "-o", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(args[i], "--output", StringComparison.OrdinalIgnoreCase))
            {
                return args[i + 1];
            }
        }

        var timestamp = DateTime.Now.ToString("yyyyMMdd-HHmmss");
        return Path.Combine(Directory.GetCurrentDirectory(), $"midi-device.{timestamp}.bin");
    }
}
