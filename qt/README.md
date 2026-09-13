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
```

1. Select **USB MIDI Interface** (or your interface) for input and output.
2. Set **Device ID** to match the GP-16 unit number (`0x00` for channel 1 on the unit we tested).
3. **Open ports**, then optionally **Request all patches** or start a panel bulk dump and watch the log.

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
    MainWindow.{h,cpp}     # simple port UI + log
    MidiService.{h,cpp}    # libremidi wrapper, Qt signals
    RolandSysex.{h,cpp}    # checksum, RQ1/DT1 helpers
```

## Notes

- MIDI callbacks are marshalled to the Qt main thread with `QMetaObject::invokeMethod`.
- SysEx is **not** ignored (`ignore_sysex = false`).
- Default device ID is **`0x00`** (matches the panel dump capture).
- This is a scaffold: patch models and effect UI from the C# app can be ported next.

## Live edit / SOUND CHANGE REQUEST (Phase 4)

Play Mode **needs** a SOUND CHANGE REQUEST at temporary address `00 00 75` after temp-buffer DT1s for the change to be audible. Compressor sustain writes to `00 00 11` alone were silent; the same writes followed 50 ms later by `00 00 75` were heard. Probe: `gp16-dump --poke -o "USB MIDI" -d 00`.
