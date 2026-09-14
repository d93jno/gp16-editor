# GP-16 Editor (Qt + libremidi)

C++20 / Qt 6 Widgets skeleton for the Roland GP-16 SysEx editor, using **[libremidi](https://github.com/celtera/libremidi)** for cross-platform MIDI I/O (ALSA on Linux).

## Dependencies

| Package | Purpose |
|---------|---------|
| CMake ≥ 3.22 | Build |
| C++20 compiler | g++ 12+ / clang 15+ |
| **Qt 6** (Core, Gui, Widgets) | UI |
| **libasound2-dev** (Linux) | ALSA for libremidi |
| Git | FetchContent downloads libremidi |

### Ubuntu / Debian

```bash
sudo apt install build-essential cmake git \
  qt6-base-dev libasound2-dev
```

`libremidi` is pulled automatically via CMake `FetchContent` (no system package required).

## Build

```bash
cd qt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

If Qt is installed in a non-standard prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

## Run GUI

```bash
./build/gp16-editor-qt
# or open a capture immediately (no hardware required)
./build/gp16-editor-qt ../captures/dump-20260730-153932.bin
```

The window is a librarian and one-effect-at-a-time editor, not a debug console: toolbar, a 128-patch list, a patch header, a signal chain strip, one effect's parameter form, a MIDI log dock, and a status bar.

1. Select **Input** / **Output** (typically the same USB MIDI interface) and **Device ID** (`0x00` for unit/channel 1).
2. **Refresh** ports if the interface was plugged in after launch, then **Connect**.
3. Load patches with any of the three ingest paths below. Click a row to show its Roland ID and name in the header. Search filters the list live.
4. Click a chip in the **signal chain** row to open that effect's parameter form below it. The checkbox on each chip toggles the effect on/off. Disabled slots stay visible, dimmed, so the layout never jumps.
5. Drag a slider or spin box in the form to edit the selected patch live (see **Live edit** below).

### Ingest paths (all fill the same 128-slot bank)

| Action | What it does |
|--------|----------------|
| **Dump** | Host RQ1 for Group A then Group B (50 ms gap). Needs both ports open. |
| **Listen** | Collects panel DT1s (`0F <idx> 00`) until you uncheck Listen, or until 128 patches arrive. Needs input open. |
| **Open file** | Reads a captured `.bin` (panel or RQ1 shape, auto-detected). Works with the unit unplugged. |

Starting any of the three (or Connect) drops any live-edit burst still queued behind the coalescing timer, so a stale parameter write or a trailing SOUND CHANGE REQUEST can never land after the librarian has moved on.

The **MIDI log** dock can be hidden and restored from **View → MIDI log**. Dump/edit progress and the last error land in the status bar and log.

Patch names are local-only in this pass — they are never written back to the device.

## Live edit (Phase 6)

Dragging a slider, moving a spin box, or toggling a chain chip updates the local `Patch` model immediately, then queues a DT1 write to the temporary buffer (`00 00 <offset>`) on a per-address, last-value-wins, 40 ms coalescing timer — a slider drag or a burst of chip toggles produces one message per address per tick, not one per event. Two-byte parameters (MSB/LSB pairs) send as a single DT1 with both data bytes. Once a burst settles (a tick with nothing queued), the editor sends a SOUND CHANGE REQUEST (`00 00 75`) exactly once — required for Play Mode to make the change audible, see **Live edit / SOUND CHANGE REQUEST** below.

Live sends are skipped, and the local edit still applies, when:

- the output port is closed (offline editing), or
- an RQ1/panel dump is in progress (`Dump` or `Listen`), so parameter DT1s never interleave with dump traffic.

Chain on/off writes both `0x0D` and `0x0E` (the full effect on/off bitmap) through the same path, since toggling one effect can only change bits inside those two bytes.

## CLI full dump

```bash
# List ports
./build/gp16-dump --list

# Default: listen for panel bulk dump (recommended / proven path)
./build/gp16-dump -i "USB MIDI" -f gp16-full-dump.bin -v
# then start MIDI bulk dump / transmit on the GP-16

# Host-initiated dump (two RQ1s: Group A `01 00 00`, Group B `01 40 00`, size `00 40 00`)
./build/gp16-dump --request -i "USB MIDI" -o "USB MIDI" -d 00 -f gp16-full-dump.bin -v

# Offline: decode a captured .bin (shape auto-detected), no hardware needed
./build/gp16-dump --decode captures/dump-20260730-153932.bin

# Play Mode SOUND CHANGE REQUEST probe (compressor sustain ± 0x75)
./build/gp16-dump --poke -o "USB MIDI" -d 00 -v
```

`--decode` prints all 128 rows (index, Roland ID, name, chain order, effect on/off flags) from a captured `.bin` and is the offline acceptance check for the shared parsing layer — the same `PatchBank` code path the GUI's **Open file** action uses.

`--request` matches the working Windows editor capture (`captures/MIDI_CAPTURE.md`): 3-byte address and size, then DT1 payloads accumulated to 8192 bytes per group.

Writes a concatenated SysEx `.bin` plus a `.txt` name summary.

### Patch SysEx layout

RQ1 (host → GP-16), 13 bytes:

```text
F0 41 <dev> 2A 11  [addr0 addr1 addr2]  [size0 size1 size2]  <checksum> F7
```

Panel bulk dump DT1 (one of 128):

```text
F0 41 <dev> 2A 12 0F <idx> 00  [117 data bytes]  <checksum> F7
```

- Device ID default **`00`** (unit/channel 1)
- Name: 16 ASCII chars at payload offset **0x64**
- Effect order: first **11** payload bytes

## Layout

```text
qt/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    MainWindow.{h,cpp}        # main window: toolbar, splitter, log dock, live-edit coalescing
    PatchListPanel.{h,cpp}    # search + 128-row patch list (A11 … B88)
    SignalChainWidget.{h,cpp} # two rows of on/off chips, joint-data order
    EffectEditor.{h,cpp}      # QStackedWidget parameter form for the selected slot
    EffectSpecs.{h,cpp}       # per-effect + global (Master Volume, Output Channel) ParamSpec tables
    PatchChartParser.{h,cpp}  # lenient .PCH chart parser (Phase 1)
    MidiService.{h,cpp}       # libremidi wrapper, Qt signals
    Patch.{h,cpp}             # one patch (name, chain, on/off, parameters)
    PatchBank.{h,cpp}         # 128 slots; panel + RQ1 ingest
    RolandSysex.{h,cpp}       # checksum, RQ1/DT1 helpers
    cli/gp16_dump.cpp         # CLI dump / --decode / --poke
  tests/
    test_patch_parsing.cpp
    test_librarian.cpp
    test_signal_chain.cpp
    test_effect_editor.cpp
    test_main_window.cpp
```

## Notes

- MIDI callbacks are marshalled to the Qt main thread with `QMetaObject::invokeMethod`.
- SysEx is **not** ignored (`ignore_sysex = false`).
- Default device ID is **`0x00`** (matches the panel dump capture).
- Offline check: `gp16-dump --decode captures/dump-20260730-153932.bin` and the librarian **Open file** action both use `PatchBank`.

## Live edit / SOUND CHANGE REQUEST (Phase 4)

Play Mode **needs** a SOUND CHANGE REQUEST at temporary address `00 00 75` after temp-buffer DT1s for the change to be audible. Compressor sustain writes to `00 00 11` alone were silent; the same writes followed 50 ms later by `00 00 75` were heard. Probe: `gp16-dump --poke -o "USB MIDI" -d 00`.
