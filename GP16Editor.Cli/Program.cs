using GP16Editor.Models;
using GP16Editor.Core;

Console.WriteLine("GP-16 Editor CLI");
Console.WriteLine("----------------");

var interactiveMode = args.Any(arg =>
    string.Equals(arg, "-i", StringComparison.OrdinalIgnoreCase) ||
    string.Equals(arg, "-interactive", StringComparison.OrdinalIgnoreCase) ||
    string.Equals(arg, "--interactive", StringComparison.OrdinalIgnoreCase));
var (includeBankA, includeBankB, bankSelectionError) = ParseBankSelection(args);
if (bankSelectionError is not null)
{
    Console.WriteLine(bankSelectionError);
    return;
}

var sysExService = new SysExService();
using var midiService = new MidiService(sysExService);
midiService.DeviceId = 0x00; // Device ID 0

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
    Console.WriteLine($"  {i}: {inputDevices[i]}");
}

int defaultInputIndex = inputDevices.FindIndex(d => d.Contains("MIDI", StringComparison.OrdinalIgnoreCase));
if (defaultInputIndex == -1) defaultInputIndex = 0;

Console.Write($"Select input device (number) [default {defaultInputIndex}]: ");
int inputDeviceIndex;
while (true)
{
    var line = Console.ReadLine();
    if (string.IsNullOrWhiteSpace(line))
    {
        inputDeviceIndex = defaultInputIndex;
        break;
    }
    if (int.TryParse(line, out inputDeviceIndex) && inputDeviceIndex >= 0 && inputDeviceIndex < inputDevices.Count)
        break;
    Console.Write("Invalid selection. Please select input device (number): ");
}

Console.WriteLine("Available MIDI Output Devices:");
for (int i = 0; i < outputDevices.Count; i++)
{
    Console.WriteLine($"  {i}: {outputDevices[i]}");
}

int defaultOutputIndex = outputDevices.FindIndex(d => d.Contains("MIDI", StringComparison.OrdinalIgnoreCase));
if (defaultOutputIndex == -1) defaultOutputIndex = 0;

Console.Write($"Select output device (number) [default {defaultOutputIndex}]: ");
int outputDeviceIndex;
while (true)
{
    var line = Console.ReadLine();
    if (string.IsNullOrWhiteSpace(line))
    {
        outputDeviceIndex = defaultOutputIndex;
        break;
    }
    if (int.TryParse(line, out outputDeviceIndex) && outputDeviceIndex >= 0 && outputDeviceIndex < outputDevices.Count)
        break;
    Console.Write("Invalid selection. Please select output device (number): ");
}

midiService.SelectDevices(inputDevices[inputDeviceIndex], outputDevices[outputDeviceIndex]);

Console.WriteLine($"\nConnected: IN='{inputDevices[inputDeviceIndex]}'  OUT='{outputDevices[outputDeviceIndex]}'");
Console.WriteLine($"Requesting patch dump from GP-16 (Banks: {DescribeSelectedBanks(includeBankA, includeBankB)})...\n");

var patchService = new PatchService(midiService, sysExService);
var progressDisplayWidth = 0;
var progress = new Progress<int>(patchNumber =>
{
    var progressText = $"  Received patch {patchNumber}...";
    if (progressText.Length < progressDisplayWidth)
    {
        progressText = progressText.PadRight(progressDisplayWidth);
    }
    else
    {
        progressDisplayWidth = progressText.Length;
    }

    Console.Write($"\r{progressText}");
});

List<Patch> patches;
try
{
    patches = await patchService.GetPatchesAsync(includeBankA, includeBankB, progress);
}
catch (Exception ex)
{
    Console.WriteLine($"\nError during dump: {ex.Message}");
    return;
}

if (progressDisplayWidth > 0)
{
    Console.Write($"\r{new string(' ', progressDisplayWidth)}\r");
}

Console.WriteLine($"\nDump complete — {patches.Count} patches received.\n");
var patchEntries = BuildPatchEntries(patches, includeBankA, includeBankB);

if (interactiveMode)
{
    var selectedPatchCode = PromptForPatchSelection(patchEntries, includeBankA, includeBankB);
    var selectedPatch = patchEntries.First(e => string.Equals(e.Code, selectedPatchCode, StringComparison.OrdinalIgnoreCase));
    Console.WriteLine($"Detailed patch dump for {selectedPatchCode}");
    Console.WriteLine(new string('-', 70));
    PrintPatchDetails(selectedPatch.Patch, selectedPatchCode, selectedPatch.AbsoluteIndex);
}
else
{
    Console.WriteLine($"{"#",-4} {"Code",-5} {"Name",-18} {"Comp Sus",-10} {"Comp Atk",-10} {"Drive",-8} {"Turbo",-6}");
    Console.WriteLine(new string('-', 68));

    for (int i = 0; i < patchEntries.Count; i++)
    {
        var entry = patchEntries[i];
        var p = entry.Patch;
        Console.WriteLine($"{i + 1,-4} {entry.Code,-5} {p.PatchName,-18} {p.Compressor.Sustain,-10} {p.Compressor.Attack,-10} {p.DistortionOverdrive.Drive,-8} {p.DistortionOverdrive.Turbo,-6}");
    }
}

Console.WriteLine("\nDone.");
return;

string PromptForPatchSelection(List<(string Code, Patch Patch, int AbsoluteIndex)> entries, bool includeA, bool includeB)
{
    var availablePatchCodes = entries.Select(e => e.Code).ToHashSet(StringComparer.OrdinalIgnoreCase);
    Console.Write("Select patch to dump ([A|B]xy where x=bank 1-8 and y=patch 1-8): ");
    while (true)
    {
        var input = Console.ReadLine();
        if (TryParsePatchSelection(input, out var patchCode))
        {
            if (availablePatchCodes.Contains(patchCode))
            {
                return patchCode;
            }

            Console.Write($"Patch {patchCode} is not in selected banks ({DescribeSelectedBanks(includeA, includeB)}). Select another: ");
            continue;
        }

        Console.Write("Invalid selection. Use format [A|B]xy (examples: A11, A84, B37): ");
    }
}

bool TryParsePatchSelection(string? input, out string patchCode)
{
    patchCode = string.Empty;
    if (string.IsNullOrWhiteSpace(input))
    {
        return false;
    }

    var trimmed = input.Trim().ToUpperInvariant();
    if (trimmed.Length != 3)
    {
        return false;
    }

    var group = trimmed[0];
    if (group != 'A' && group != 'B')
    {
        return false;
    }

    var bankChar = trimmed[1];
    var patchChar = trimmed[2];
    if (bankChar < '1' || bankChar > '8' || patchChar < '1' || patchChar > '8')
    {
        return false;
    }

    var bank = bankChar - '0';
    var patch = patchChar - '0';
    patchCode = $"{group}{bank}{patch}";
    return true;
}

List<(string Code, Patch Patch, int AbsoluteIndex)> BuildPatchEntries(List<Patch> loadedPatches, bool includeA, bool includeB)
{
    var entries = new List<(string Code, Patch Patch, int AbsoluteIndex)>();
    var index = 0;

    if (includeA)
    {
        for (int bank = 1; bank <= 8 && index < loadedPatches.Count; bank++)
        {
            for (int patch = 1; patch <= 8 && index < loadedPatches.Count; patch++)
            {
                var code = $"A{bank}{patch}";
                var absoluteIndex = ((bank - 1) * 8) + patch;
                entries.Add((code, loadedPatches[index], absoluteIndex));
                index++;
            }
        }
    }

    if (includeB)
    {
        for (int bank = 1; bank <= 8 && index < loadedPatches.Count; bank++)
        {
            for (int patch = 1; patch <= 8 && index < loadedPatches.Count; patch++)
            {
                var code = $"B{bank}{patch}";
                var absoluteIndex = 64 + ((bank - 1) * 8) + patch;
                entries.Add((code, loadedPatches[index], absoluteIndex));
                index++;
            }
        }
    }

    return entries;
}

(bool IncludeA, bool IncludeB, string? Error) ParseBankSelection(string[] commandLineArgs)
{
    string? banksArgument = null;
    for (int i = 0; i < commandLineArgs.Length; i++)
    {
        var arg = commandLineArgs[i];
        if (string.Equals(arg, "-b", StringComparison.OrdinalIgnoreCase) || string.Equals(arg, "--banks", StringComparison.OrdinalIgnoreCase))
        {
            if (i + 1 >= commandLineArgs.Length)
            {
                return (false, false, "Missing value for -b. Use -b A, -b B, or -b A,B.");
            }

            banksArgument = commandLineArgs[++i];
            continue;
        }

        if (arg.StartsWith("-b=", StringComparison.OrdinalIgnoreCase))
        {
            banksArgument = arg[3..];
            continue;
        }

        if (arg.StartsWith("--banks=", StringComparison.OrdinalIgnoreCase))
        {
            banksArgument = arg[8..];
        }
    }

    if (banksArgument is null)
    {
        return (true, true, null);
    }

    if (string.IsNullOrWhiteSpace(banksArgument))
    {
        return (false, false, "Invalid -b value. Use -b A, -b B, or -b A,B.");
    }

    var includeA = false;
    var includeB = false;
    var tokens = banksArgument.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
    if (tokens.Length == 0)
    {
        return (false, false, "Invalid -b value. Use -b A, -b B, or -b A,B.");
    }

    foreach (var token in tokens)
    {
        switch (token.ToUpperInvariant())
        {
            case "A":
                includeA = true;
                break;
            case "B":
                includeB = true;
                break;
            default:
                return (false, false, $"Unknown bank '{token}'. Use -b A, -b B, or -b A,B.");
        }
    }

    if (!includeA && !includeB)
    {
        return (false, false, "At least one bank must be selected. Use -b A, -b B, or -b A,B.");
    }

    return (includeA, includeB, null);
}

string DescribeSelectedBanks(bool includeA, bool includeB)
{
    return includeA && includeB ? "A,B" : includeA ? "A" : "B";
}

void PrintPatchDetails(Patch patch, string patchCode, int absoluteIndex)
{
    Console.WriteLine($"Patch: {patchCode}");
    Console.WriteLine($"Absolute Index: {absoluteIndex}");
    Console.WriteLine($"Name: {patch.PatchName}");
    Console.WriteLine($"Device Patch Number: {patch.PatchNumber}");
    Console.WriteLine($"Block A: {string.Join(", ", patch.BlockA)}");
    Console.WriteLine($"Block B: {string.Join(", ", patch.BlockB)}");
    Console.WriteLine($"Block B2 Mode: {patch.BlockB2ModeName} ({patch.BlockB2Mode})");
    Console.WriteLine("Effect On/Off:");
    Console.WriteLine($"  A-1 Compressor: {FormatParameterValue(patch.IsCompressorEnabled)}");
    Console.WriteLine($"  A-2 Distortion/Overdrive: {FormatParameterValue(patch.IsDistortionOverdriveEnabled)}");
    Console.WriteLine($"  A-3 Picking Filter: {FormatParameterValue(patch.IsPickingFilterEnabled)}");
    Console.WriteLine($"  A-4 Step Phaser: {FormatParameterValue(patch.IsStepPhaserEnabled)}");
    Console.WriteLine($"  A-5 Parametric EQ: {FormatParameterValue(patch.IsParametricEQEnabled)}");
    Console.WriteLine($"  A-6 Noise Suppressor: {FormatParameterValue(patch.IsNoiseSuppressorEnabled)}");
    Console.WriteLine($"  B-1 Short Delay: {FormatParameterValue(patch.IsShortDelayEnabled)}");
    Console.WriteLine($"  B-2 Selected ({patch.BlockB2ModeName}): {FormatParameterValue(patch.IsBlockB2Enabled)}");
    Console.WriteLine($"    Chorus: {FormatParameterValue(patch.IsChorusEnabled)}");
    Console.WriteLine($"    Flanger: {FormatParameterValue(patch.IsFlangerEnabled)}");
    Console.WriteLine($"    Pitch Shifter: {FormatParameterValue(patch.IsPitchShifterEnabled)}");
    Console.WriteLine($"    Space-D: {FormatParameterValue(patch.IsSpaceDEnabled)}");
    Console.WriteLine($"  B-3 Auto Panpot: {FormatParameterValue(patch.IsAutoPanpotEnabled)}");
    Console.WriteLine($"  B-4 Tap Delay: {FormatParameterValue(patch.IsTapDelayEnabled)}");
    Console.WriteLine($"  B-5 Reverb: {FormatParameterValue(patch.IsReverbEnabled)}");
    Console.WriteLine($"  B-6 Lineout Filter: {FormatParameterValue(patch.IsLineoutFilterEnabled)}");

    PrintParameters("Compressor", patch.Compressor);
    PrintParameters("DistortionOverdrive", patch.DistortionOverdrive);
    PrintParameters("PickingFilter", patch.PickingFilter);
    PrintParameters("StepPhaser", patch.StepPhaser);
    PrintParameters("ParametricEQ", patch.ParametricEQ);
    PrintParameters("NoiseSuppressor", patch.NoiseSuppressor);
    PrintParameters("ShortDelay", patch.ShortDelay);
    PrintParameters("Chorus", patch.Chorus);
    PrintParameters("Flanger", patch.Flanger);
    PrintParameters("PitchShifter", patch.PitchShifter);
    PrintParameters("SpaceD", patch.SpaceD);
    PrintParameters("AutoPanpot", patch.AutoPanpot);
    PrintParameters("TapDelay", patch.TapDelay);
    PrintParameters("Reverb", patch.Reverb);
    PrintParameters("LineoutFilter", patch.LineoutFilter);
}

void PrintParameters(string sectionName, object parameters)
{
    Console.WriteLine($"\n{sectionName}:");
    foreach (var property in parameters.GetType().GetProperties())
    {
        var value = property.GetValue(parameters);
        Console.WriteLine($"  {property.Name}: {FormatParameterValue(value)}");
    }
}

string FormatParameterValue(object? value)
{
    if (value is null)
    {
        return "<null>";
    }

    if (value is bool boolValue)
    {
        return boolValue ? "On" : "Off";
    }

    return value.ToString() ?? "<null>";
}
