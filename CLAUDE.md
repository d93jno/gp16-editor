# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

This is a .NET 9 MAUI solution. Use the C# Dev Kit extension in VS Code (or `dotnet` CLI) to build.

```bash
# Build the MAUI app (Windows target)
dotnet build GP16Editor/GP16Editor.csproj -f net9.0-windows10.0.19041.0

# Build the CLI tool
dotnet build GP16Editor.Cli/GP16Editor.Cli.csproj

# Run the CLI tool
dotnet run --project GP16Editor.Cli/GP16Editor.Cli.csproj

# Build entire solution
dotnet build gp16editor.sln
```

There are no automated tests in this project.

## Code Style

- No comments in code (per project convention)
- Treat all `PropertyChanged` event invocations as nullable (use `?.Invoke`)

## Architecture

Four projects with a strict dependency direction: `GP16Editor` / `GP16Editor.Cli` → `GP16Editor.Core` → `GP16Editor.Models`.

**GP16Editor.Models** — Pure data layer, no external dependencies.
- `Patch.cs` — top-level model; parses a 127-byte binary patch buffer into typed effect parameter objects
- `EffectParameters.cs` — one class per effect (15 total), all inheriting `NotifyPropertyChangedBase`
- `SysExMessages.cs` — `ParsedDT1Message`, `DT1Address`, `BulkDumpType` enum for decoded SysEx
- `NotifyPropertyChangedBase.cs` — lightweight MVVM base (`SetProperty<T>`)

**GP16Editor.Core** — MIDI communication and patch retrieval, no UI dependency.
- `MidiService` — owns the DryWetMidi `IInputDevice`/`IOutputDevice` lifecycle; fires `SysExReceived` and `ErrorOccurred` events; enforces 50ms inter-message delay required by the GP-16
- `SysExService` — stateless; builds/parses Roland DT1 (0x12) and RQ1 (0x11) SysEx frames including the 7-bit checksum
- `PatchService` — orchestrates bulk-dump requests (Group A / Group B) by sending RQ1 messages and waiting for DT1 responses; parses 128 patches
- `HexDump` — static debug utility; `HexDump.Print(bytes, label)` writes a traditional hex dump to `Debug.WriteLine`

**GP16Editor** — .NET MAUI UI, MVVM with CommunityToolkit.Mvvm.
- Services registered as singletons in `MauiProgram.cs`: `SysExService` → `MidiService` → `PatchService` → `MainViewModel`
- `MainViewModel` — central coordinator; holds `CurrentPatch`, `AllPatches`, `FilteredPatches`; owns one ViewModel instance per effect; dispatches MIDI parameter-change sends on property changes
- One `*ViewModel` per effect (15 effects) plus `ConfigurationViewModel` and `EffectSequenceBlockViewModel`
- `ConfigurationViewModel` — manages device selection and persists settings via `Preferences`

**GP16Editor.Cli** — headless console app for MIDI testing/dev without a GUI.

## Roland GP-16 SysEx Protocol

**Message format (one-way DT1):**
```
F0 41 [DeviceID] 2A 12 [Addr0] [Addr1] [Addr2] [Data...] [Checksum] F7
```
- Manufacturer: `0x41` (Roland), Model: `0x2A` (GP-16)
- Command `0x11` = RQ1 (request data), `0x12` = DT1 (set data)
- DeviceID = MIDI channel − 1 (default `0x10`)

**Checksum (7-bit Roland):**
1. Sum all address + data bytes
2. `remainder = sum % 128`
3. `checksum = remainder == 0 ? 0 : 128 − remainder`

**Memory map:**

| Area | Base Address |
|---|---|
| Temporary Buffer (active patch) | `00 00 00` |
| Internal Group A (patches 1–64) | `01 00 00` |
| Internal Group B (patches 65–128) | `02 00 00` |
| System settings | `04 00 00` |

Address bytes are 7-bit (`0x00–0x7F`); carry over to the next byte when exceeding 127.

**Key offsets (Temporary Buffer):**
- `00 00 00–0F` — Patch name (16 ASCII chars)
- Compressor: Sustain `06`, Attack `07`
- Distortion: Drive `0B`, Turbo `0C`
- Picking Filter: Cutoff `11`, Up/Down `13`
- Chorus: Pre-Delay `23`, Rate `24`, Depth `25`
- Reverb: Time `3D`, Type `3F`

Real-time edits target the Temporary Buffer and are heard immediately; they are not saved to internal memory until a write command is sent or the user saves on the device front panel.

**DryWetMidi note:** `NormalSysExEvent.Data` excludes `F0` and `F7`; prepend/append them manually when passing data to `SysExService.ParseDt1Message`.
