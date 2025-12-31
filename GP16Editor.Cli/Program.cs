using GP16Editor.Cli;

Console.WriteLine("GP-16 Editor CLI");
Console.WriteLine("---------------");

using var midiService = new MidiService();
midiService.DeviceId = 0x00; // Use MIDI channel 1 (Device ID 0)


var inputDevices = midiService.GetInputDevices().ToList();
var outputDevices = midiService.GetOutputDevices().ToList();

if (!inputDevices.Any())
{
    Console.WriteLine("No MIDI input devices found.");
    return;
}

if (!outputDevices.Any())
{
    Console.WriteLine("No MIDI output devices found.");
    return;
}

Console.WriteLine("Available MIDI Input Devices:");
for (int i = 0; i < inputDevices.Count; i++)
{
    Console.WriteLine($"{i}: {inputDevices[i]}");
}

Console.Write("Select input device (number): ");
int inputDeviceIndex;
while (!int.TryParse(Console.ReadLine(), out inputDeviceIndex) || inputDeviceIndex < 0 || inputDeviceIndex >= inputDevices.Count)
{
    Console.Write("Invalid selection. Please select input device (number): ");
}

Console.WriteLine("Available MIDI Output Devices:");
for (int i = 0; i < outputDevices.Count; i++)
{
    Console.WriteLine($"{i}: {outputDevices[i]}");
}

Console.Write("Select output device (number): ");
int outputDeviceIndex;
while (!int.TryParse(Console.ReadLine(), out outputDeviceIndex) || outputDeviceIndex < 0 || outputDeviceIndex >= outputDevices.Count)
{
    Console.Write("Invalid selection. Please select output device (number): ");
}

var selectedInputDevice = inputDevices[inputDeviceIndex];
var selectedOutputDevice = outputDevices[outputDeviceIndex];

midiService.SelectDevices(selectedInputDevice, selectedOutputDevice);
midiService.SysExReceived += (sender, e) =>
{
    var hex = string.Join(" ", e.Data.Select(b => b.ToString("X2")));
    Console.WriteLine($"SysEx Received: F0 {hex} F7");
};

Console.WriteLine($"Listening for MIDI messages on '{selectedInputDevice}'...");
Console.WriteLine("Press 'q' to quit.");

while (Console.ReadKey(true).KeyChar != 'q')
{
    // Loop until user quits
}
