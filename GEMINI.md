# GP-16 Editor — application & development guide

Guidance for humans and coding agents working in this repository.

`CLAUDE.md` and `AGENTS.md` are symlinks to this file.

**Protocol / SysEx / parameter maps:** [`docs/GP16_PROTOCOL.md`](docs/GP16_PROTOCOL.md)  
**Full address table:** [`midi_parameter_mapping.md`](midi_parameter_mapping.md) (Table 1)  
**Owner’s manual:** [`docs/GP-16_OM.pdf`](docs/GP-16_OM.pdf) ([OCR](docs/GP-16_OM.md))  
**Sample dumps:** [`captures/`](captures/) ([`MIDI_CAPTURE.md`](captures/MIDI_CAPTURE.md) + `.bin` dumps)

Always commit code changes to the source tree, never to the work tree. Only create git commits when asked. Make no attribution to Claude (or other agents) in commit messages.

---

## What this project is

Graphical / tooling stack to edit a **Roland GP-16** multi-effects unit over **MIDI SysEx** (Roland manufacturer ID `41`, model ID `2A`).

There are **two parallel implementations**:

| Tree | Role | Platforms |
|------|------|-----------|
| `qt/` | Active-development **C++20** Qt Widgets editor + **CLI dump tool** | Linux (ALSA), also portable via libremidi |
| `GP16Editor/` + `gp16editor.sln` | **Obsolete** — .NET MAUI editor (C#), superseded by `qt/`, kept for reference only | Windows, macOS (Catalyst), iOS; DryWetMidi device I/O is **Windows/macOS** |

The Qt tree (`qt/`) is where new feature work and bug fixes happen. The C# MAUI tree is no longer under active development; do not add new features there unless explicitly asked — prefer porting new work to `qt/` instead.

---

## Repository layout

```text
gp16-editor/
  GEMINI.md                 # this file (app + development instructions)
  CLAUDE.md → GEMINI.md
  AGENTS.md → GEMINI.md
  docs/GP16_PROTOCOL.md     # SysEx protocol, memory map, offsets
  midi_parameter_mapping.md # authoritative Temporary Buffer Table 1
  gp16editor.sln
  GP16Editor/               # MAUI C# app
  GP16Editor.Core/          # MIDI / SysEx / patch services
  GP16Editor.Models/        # Patch + effect parameter models
  GP16Editor.Cli/           # headless C# MIDI CLI
  qt/                       # CMake C++ project
    src/
      main.cpp
      MainWindow.*
      MidiService.*
      RolandSysex.*
      cli/gp16_dump.cpp
  captures/                 # recorded SysEx dumps
  plans/                    # implementation plans
```

---

## Coding style

### General

- Prefer clear, working code over comments; do not add narrating comments.
- Keep changes scoped to the task; do not drive-by refactor unrelated files.
- Prefer editing **one stack at a time** (do not rewrite working C# when only fixing `qt/`, and vice versa) unless asked for both.
- Roland SysEx details (checksum, addresses, delays): see [`docs/GP16_PROTOCOL.md`](docs/GP16_PROTOCOL.md). Between large messages, use a **20–50 ms** gap.
- Do not commit `qt/build/` or MAUI `bin/` / `obj/` artifacts.

### C# (`GP16Editor/`, Core, Models, Cli) — obsolete, reference only

- This tree is **obsolete** and superseded by `qt/`. Do not add new features or drive-by refactor it; only touch it if explicitly asked (e.g. bug fix requested against the C# app specifically).
- No comments in code (project convention).
- Treat all `PropertyChanged` event invocations as nullable (`?.Invoke`).
- MVVM: Views bind to ViewModels; MIDI I/O stays in services (`MidiService` / Core).
- Nullable reference types enabled; implicit usings on.
- Do **not** casually rewrite working C# MIDI paths unless explicitly asked.
- DryWetMidi: `NormalSysExEvent.Data` excludes `F0` and `F7`; prepend/append them when passing data to `SysExService.ParseDt1Message`.

### C++ (`qt/`) — active development

- C++20, no compiler extensions.
- Qt: `AUTOMOC` on; QObject MIDI callbacks marshalled to the UI thread (`QueuedConnection` / `invokeMethod`).
- Shared SysEx helpers in static lib `gp16_sysex` (`RolandSysex.*`); GUI and CLI both link it.
- Prefer **ALSA raw** (`--api alsa_raw`) for reliable SysEx with generic USB MIDI interfaces on Linux.

---

## Architecture (C# MAUI, obsolete)

Four projects with a strict dependency direction: `GP16Editor` / `GP16Editor.Cli` → `GP16Editor.Core` → `GP16Editor.Models`.

**GP16Editor.Models** — Pure data layer, no external dependencies.
- `Patch.cs` — top-level model; parses a binary patch buffer into typed effect parameter objects
- `EffectParameters.cs` — one class per effect (15 total), all inheriting `NotifyPropertyChangedBase`
- `SysExMessages.cs` — `ParsedDT1Message`, `DT1Address`, `BulkDumpType` enum for decoded SysEx
- `NotifyPropertyChangedBase.cs` — lightweight MVVM base (`SetProperty<T>`)

**GP16Editor.Core** — MIDI communication and patch retrieval, no UI dependency.
- `MidiService` — DryWetMidi `IInputDevice`/`IOutputDevice` lifecycle; `SysExReceived` / `ErrorOccurred`; 50 ms inter-message delay
- `SysExService` — builds/parses Roland DT1 (`0x12`) and RQ1 (`0x11`) including the 7-bit checksum
- `PatchService` — bulk-dump requests (Group A / Group B); parses 128 patches
- `HexDump` — `HexDump.Print(bytes, label)` → `Debug.WriteLine`

**GP16Editor** — .NET MAUI UI, MVVM with CommunityToolkit.Mvvm.
- Singletons in `MauiProgram.cs`: `SysExService` → `MidiService` → `PatchService` → `MainViewModel`
- `MainViewModel` — `CurrentPatch`, `AllPatches`, `FilteredPatches`; one ViewModel per effect; MIDI sends on property changes
- `ConfigurationViewModel` — device selection; persists via `Preferences`

**GP16Editor.Cli** — headless console app for MIDI testing without a GUI.

There are no automated tests in this project.

---

## Tools and dependencies

### C# MAUI

- .NET 9 SDK with MAUI workloads for the target OS
- C# Dev Kit / Visual Studio (or `dotnet` CLI)
- NuGet: `Melanchall.DryWetMidi` 8.x, `Microsoft.Maui.Controls`, SkiaSharp
- TFMs: `net9.0-ios`, `net9.0-maccatalyst`, and on Windows `net9.0-windows10.0.19041.0`
- DryWetMidi **InputDevice / OutputDevice** work on **Windows and macOS only** (not Linux)

### Qt / C++ (`qt/`)

| Need | Notes |
|------|--------|
| CMake ≥ 3.22 | |
| C++20 | g++ 12+ / clang 15+ |
| Qt 6 | Core, Gui, Widgets (`qt6-base-dev` on Debian/Ubuntu) |
| ALSA | `libasound2-dev` on Linux |
| Git | `FetchContent` pulls **libremidi** v5.3.1 |

```bash
sudo apt install build-essential cmake git qt6-base-dev libasound2-dev
```

---

## Build & run

### MAUI editor (C#)

```bash
dotnet build GP16Editor/GP16Editor.csproj -f net9.0-windows10.0.19041.0
dotnet run --project GP16Editor/GP16Editor.csproj -f net9.0-windows10.0.19041.0

dotnet build GP16Editor.Cli/GP16Editor.Cli.csproj
dotnet run --project GP16Editor.Cli/GP16Editor.Cli.csproj

dotnet build gp16editor.sln
```

In the app: pick MIDI input/output → Request Patch (RQ1) and/or listen while the GP-16 panel dumps. Device ID default is `0x00`.

### Qt GUI (`gp16-editor-qt`)

```bash
cd qt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/gp16-editor-qt
```

Optional: `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64`.

In the UI: select ports → Device ID `0x00` → Open ports → log / request / panel dump.

### CLI dump tool (`gp16-dump`)

```bash
cd qt
cmake --build build -j --target gp16-dump
./build/gp16-dump --list
```

Panel listen (recommended on Linux):

```bash
./build/gp16-dump -i "USB MIDI" -f gp16-full-dump.bin -v
# then on the GP-16: MIDI bulk dump / transmit all
```

Host RQ1 (may get no reply on some Linux + generic USB MIDI setups):

```bash
./build/gp16-dump --request -i "USB MIDI" -o "USB MIDI" -d 00 -f gp16-full-dump.bin
```

Useful flags: `--listen-seconds N`, `--api alsa_raw|alsa_seq`, `-d <hex>`, `--from` / `--to`.

### MIDI health check (Linux)

```bash
amidi -l
cat /proc/asound/card*/midi*
amidi -p hw:X,0,0 --dump
```

If the interface TX lamp blinks but Rx bytes never increase, the host is sending but IN/SysEx is not delivering. Prefer a direct motherboard USB port for flaky `fc02:0101`-class adapters.

After MIDI changes on Linux, verify with `gp16-dump --list`, Tx/Rx counters, and ideally a panel dump listen.
