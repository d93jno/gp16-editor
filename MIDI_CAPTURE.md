# MIDI capture — bulk patch dump

How the Windows MAUI editor records a full Group A + Group B SysEx dump, and how to decode the resulting `.bin` files (e.g. on Linux for comparison).

Source of truth: `MidiService`, `PatchService`, `SysExService`, `MainViewModel.RefreshPatches`. Protocol background: `docs/GP16_PROTOCOL.md`, `midi_parameter_mapping.md`, `gp16_pages_67_76.md`. App/dev guidance: `GEMINI.md`.

---

## 1. How capture is triggered

Capture only runs when the environment variable `GP16EDITOR_CAPTURE_MIDI` is set to `1`. Without it, refresh behaves normally and no `.bin` files are written or alert shown — this keeps the feature available for future debugging without it firing on every refresh.

1. User clicks the top-bar refresh control → `RefreshPatchesCommand` → `MainViewModel.RefreshPatches`.
2. Requires MIDI in/out connected (`MidiService.IsConnected`); otherwise an alert and no capture.
3. Progress popup opens (128 patches / 16384 expected data bytes).
4. If `GP16EDITOR_CAPTURE_MIDI=1`:
   a. Timestamp and output paths are chosen (see §2).
   b. `_midiService.StartRecording()` clears buffers and enables capture.
5. `_patchService.GetAllPatchesAsync(...)` runs a full dump (Group A then Group B).
6. In a `finally` block, if capture was enabled: `StopRecording()` → write both `.bin` files → close popup.
7. If capture was enabled, alert shows the two file paths.

Recording is **only** active for that dump window. Parameter edits and other SysEx outside refresh are not captured.

---

## 2. File naming and location

| File | Contents |
|------|----------|
| `midi-out-<yyyyMMdd-HHmmss>.bin` | Concatenated **outgoing** SysEx (app → GP-16) — hyphen after `out` |
| `midi-in.<yyyyMMdd-HHmmss>.bin` | Concatenated **incoming** SysEx (GP-16 → app) — **period** after `in` (not a hyphen) |

Exact patterns from `MainViewModel.RefreshPatches`:

```csharp
$"midi-out-{timestamp}.bin"   // hyphen
$"midi-in.{timestamp}.bin"    // period
```

`timestamp` = `DateTime.Now.ToString("yyyyMMdd-HHmmss")`.

Directory:

1. `Environment.SpecialFolder.DesktopDirectory` (Desktop)
2. If empty/unavailable: `FileSystem.Current.AppDataDirectory`

Both files share the same timestamp from the start of that refresh.

---

## 3. Binary file format

- **Not** a custom container, SMF, or length-prefixed stream.
- Raw MIDI bytes: one or more SysEx messages concatenated end-to-end.
- Each message is `F0 … F7`.
- Split by scanning for `F0`, then reading until the matching `F7`.

Outgoing: bytes are stored exactly as built/sent (already include `F0`/`F7`).

Incoming: DryWetMidi’s `NormalSysExEvent.Data` often omits `F0`/`F7`; `MidiService` reconstructs a full frame before appending (see §7).

---

## 4. What is sent (`midi-out-*.bin`)

`GetAllPatchesAsync` → `GetPatchesAsync(includeGroupA: true, includeGroupB: true)` sends **two RQ1** messages via `MidiService.RequestDataDump` → `SysExService.BuildRq1Message`.

| Order | Group | Address | Size |
|-------|--------|---------|------|
| 1 | Internal Group A (patches 1–64) | `01 00 00` | `00 40 00` |
| 2 | Internal Group B (patches 65–128) | `01 40 00` | `00 40 00` |

RQ1 frame (`COMMAND_ID_RQ1 = 0x11`):

```
F0 41 [DeviceId] 2A 11 [Addr0] [Addr1] [Addr2] [Size0] [Size1] [Size2] [Checksum] F7
```

Constants (`SysExService`): Manufacturer `0x41`, Model `0x2A`.

`DeviceId` is `MidiService.DeviceId` (default `0` in code). Must match the GP-16 unit number (MIDI channel − 1).

Checksum over address + size only (see §6). After each send, `SendSysExAsync` waits **50 ms**.

A normal capture’s out file is typically exactly these two RQ1 frames (24 bytes each if DeviceId is one byte as usual).

---

## 5. What is received (`midi-in.*.bin`)

GP-16 answers with a sequence of **DT1** messages (`0x12`). Roland splits large dumps into multiple DT1 frames (manual notes ~256-byte limit per DT1).

DT1 frame:

```
F0 41 [DeviceId] 2A 12 [Addr0] [Addr1] [Addr2] [Data…] [Checksum] F7
```

`PatchService` handler (same dump):

1. Reconstruct full SysEx (`F0`/`F7` if missing).
2. If command byte is DT1, `SysExService.ParseDt1Message`.
3. On valid parse, append **payload only** (`parsedMessage.Data` — not address/checksum) to a per-group buffer.
4. When buffer length ≥ **8192**, that group is complete; then Group B is requested the same way.

Expected totals:

- Per group: 8192 data bytes
- Full refresh: 16384 data bytes (progress UI uses this)

### Patch layout in the accumulated buffer

Constants in `PatchService`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `PATCH_SIZE` | `0x7F` (127) | Bytes taken as one patch |
| `PATCH_OFFSET` | `0x80` (128) | Stride between patches |
| `PATCHES_PER_GROUP` | 64 | Per group |
| `TOTAL_PATCHES` | 128 | A + B |

For patch index `i` in a group buffer:

```
patchBytes = buffer[i * 0x80 .. i * 0x80 + 0x7F)   // 127 bytes; byte at +0x7F unused in parse
```

Then `new Patch().ParsePatchData(patchBytes)`.

Group A → patches 0–63; Group B → 64–127 in the combined list. Memory map (`docs/GP16_PROTOCOL.md`): both groups share area byte `01`; Group A base `01 00 00`, Group B base `01 40 00` (there is no separate `02` area byte — Group B is just the `01` area with a `0x40` patch-number offset). Temporary buffer `00 00 00` is not part of this dump.

---

## 6. How to decode

### Roland 7-bit checksum

Matches `SysExService.CalculateChecksum`:

1. Sum all address + data (or address + size for RQ1) bytes.
2. `remainder = sum % 128`
3. `checksum = (128 - remainder) % 128`  
   (so if remainder is 0, checksum is 0)

Verify DT1: checksum is the byte immediately before `F7`; body = 3 address bytes + data payload.

### Parse concatenated SysEx

1. Split `midi-in.*.bin` / `midi-out-*.bin` into `F0…F7` messages.
2. Out: expect two RQ1s with addresses/sizes above.
3. In: for each DT1, validate header `F0 41 … 2A 12`, checksum, extract payload = bytes after 3-byte address through before checksum.
4. Concatenate payloads in order for Group A until 8192 bytes; then Group B the same (order follows the two RQ1s).
5. Slice each 8192-byte buffer with stride `0x80`, length `0x7F` → 64 patches × 2 groups.

### Existing parsers / docs

| Piece | Location |
|-------|----------|
| Build/parse RQ1 & DT1, checksum | `GP16Editor.Core/SysExService.cs` |
| Dump orchestration, 8192 / patch stride | `GP16Editor.Core/PatchService.cs` |
| DT1 → typed fields | `SysExService.ParseDt1Message` → `Models.ParsedDT1Message` |
| 127-byte patch → effects | `GP16Editor.Models/Patch.ParsePatchData` |
| Parameter / address tables | `midi_parameter_mapping.md`, `gp16_pages_67_76.md` |
| Protocol summary | `docs/GP16_PROTOCOL.md` |
| App / development guide | `GEMINI.md` |

---

## 7. DryWetMidi quirk

`NormalSysExEvent.Data` **excludes** leading `F0` and trailing `F7`.

- **Send:** `SendSysExAsync` strips `F0`/`F7` for DryWetMidi, but **records the full message** (with `F0`/`F7`) into the out buffer.
- **Receive:** `OnEventReceived` records via `ToFullSysExMessage`: prepend `F0` if missing, append `F7` if missing. `PatchService` does the same reconstruction before `ParseDt1Message`.

Captured `.bin` files always use full `F0…F7` frames. A Linux ALSA/`amidi` capture that already includes `F0`/`F7` should match after message boundaries are aligned.

---

## 8. Linux comparison tips

- Compare **byte-identical** SysEx bodies where possible: same DeviceId, same RQ1 address/size, same patch data on the unit.
- Out file: usually two RQ1s; timing gaps are not stored (only bytes).
- In file: number of DT1 frames may vary by firmware/path; **concatenated DT1 payloads** per group should still reach 8192 bytes each if the dump succeeded.
- Honor **≥ 50 ms** between outbound SysEx when replaying requests (`MidiService.SendSysExAsync`).
- Ignore UI progress / console logs; only the `.bin` bytes matter for decode.
- If DeviceId differs between hosts, RQ1/DT1 headers differ even when patch payloads match — normalize or mask byte index 2 (`DeviceId`) when comparing headers only.
