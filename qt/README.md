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

The window is a librarian, not a debug console: toolbar, 128-patch list, patch header, a MIDI log dock, and a status bar.

1. Select **Input** / **Output** (typically the same USB MIDI interface) and **Device ID** (`0x00` for unit/channel 1).
2. **Refresh** ports if the interface was plugged in after launch, then **Connect**.
3. Load patches with any of the three ingest paths below. Click a row to show its Roland ID and name in the header. Search filters the list live.

### Ingest paths (all fill the same 128-slot bank)

| Action | What it does |
|--------|----------------|
| **Dump** | Host RQ1 for Group A then Group B (50 ms gap). Needs both ports open. |
| **Listen** | Collects panel DT1s (`0F <idx> 00`) until you uncheck Listen, or until 128 patches arrive. Needs input open. |
| **Open file** | Reads a captured `.bin` (panel or RQ1 shape, auto-detected). Works with the unit unplugged. |

The **MIDI log** dock can be hidden and restored from **View → MIDI log**. Dump progress and the last error land in the status bar.

Patch names are local-only in this pass — they are never written back to the device.

## CLI full dump

```bash
# List ports
./build/gp16-dump --list

# Default: listen for panel bulk dump (recommended / proven path)
./build/gp16-dump -i "USB MIDI" -f gp16-full-dump.bin -v
# then start MIDI bulk dump / transmit on the GP-16

# Host-initiated dump (two RQ1s: Group A `01 00 00`, Group B `01 40 00`, size `00 40 00`)
./build/gp16-dump --request -i "USB MIDI" -o "USB MIDI" -d 00 -f gp16-full-dump.bin -v

# Play Mode SOUND CHANGE REQUEST probe (compressor sustain ± 0x75)
./build/gp16-dump --poke -o "USB MIDI" -d 00 -v
```

`--request` matches the working Windows editor capture (`MIDI_CAPTURE.md`): 3-byte address and size, then DT1 payloads accumulated to 8192 bytes per group.

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
    MainWindow.{h,cpp}        # librarian window: toolbar, splitter, log dock
    PatchListPanel.{h,cpp}    # search + 128-row patch list (A11 … B88)
    MidiService.{h,cpp}       # libremidi wrapper, Qt signals
    Patch.{h,cpp}             # one patch (name, chain, on/off, parameters)
    PatchBank.{h,cpp}         # 128 slots; panel + RQ1 ingest
    RolandSysex.{h,cpp}       # checksum, RQ1/DT1 helpers
    cli/gp16_dump.cpp         # CLI dump / --decode / --poke
  tests/
    test_patch_parsing.cpp
    test_librarian.cpp
```

## Notes

- MIDI callbacks are marshalled to the Qt main thread with `QMetaObject::invokeMethod`.
- SysEx is **not** ignored (`ignore_sysex = false`).
- Default device ID is **`0x00`** (matches the panel dump capture).
- Offline check: `gp16-dump --decode captures/dump-20260730-153932.bin` and the librarian **Open file** action both use `PatchBank`.

## Live edit / SOUND CHANGE REQUEST (Phase 4)

Play Mode **needs** a SOUND CHANGE REQUEST at temporary address `00 00 75` after temp-buffer DT1s for the change to be audible. Compressor sustain writes to `00 00 11` alone were silent; the same writes followed 50 ms later by `00 00 75` were heard. Probe: `gp16-dump --poke -o "USB MIDI" -d 00`.
