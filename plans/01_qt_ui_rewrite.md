# 01 — Qt UI rewrite: from debug console to functional editor

Status: in progress
Scope: `qt/` tree only. The C# MAUI app is not touched.

## Goal

Turn `qt/src/MainWindow.cpp` (a MIDI port form + three buttons + a log pane) into a working
GP-16 editor: patch librarian, signal chain navigation, one effect parameter page at a time,
and debounced live edits to the temporary buffer.

Deliberately **not** a port of the MAUI layout. `GP16Editor/MainPage.xaml:11` is a 300px list
beside a single `ScrollView`, and `:46` wraps every enabled effect as a knob card in a
`FlexLayout Wrap="Wrap"`. Cards appear and disappear as effects toggle, so the layout jumps;
EQ and Tap Delay blow out the wrap; MIDI settings hide in a popup. None of that comes across.

## Target layout

Hardware metaphor: librarian + pedalboard chain + one effect at a time. Qt 6 Widgets, Fusion
defaults, sliders and spin boxes. No custom knobs, no heavy QSS, no QML in this pass.

```
QMainWindow
├── QToolBar (always visible)
│     Input | Output | Device ID | Refresh | Connect | Dump (RQ1) | Listen | Open file
├── QSplitter (horizontal)
│     ├── Left (~240px): search field + QListWidget of 128 patches
│     └── Right
│           ├── Patch header: Roland ID + name
│           ├── Signal chain, two rows of chips in joint-data order
│           │     Block A: [ ] [ ] [ ] [ ] [ ] [Noise Supp.]
│           │     Block B: [ ] [ ] [ ] [ ] [ ] [Lineout Flt.]
│           └── QStackedWidget: parameter form for the selected slot only
│                 QFormLayout rows: label | QSlider | QSpinBox (or combo / check)
├── QDockWidget "MIDI log" (bottom, collapsible — keep it, Linux SysEx is finicky)
└── QStatusBar: port state / dump progress / last error
```

Why this is functional rather than pretty: one effect visible means every control has room;
the chain is the navigation instead of a second copy of the editors; disabled slots stay
visible but dimmed so the layout never jumps; B-2 and A-2 show the active variant in the chip
itself; MIDI state is always on screen.

## Decisions taken

| Question | Decision |
|---|---|
| Panel "Listen" dump in this pass? | Yes — GUI supports both RQ1 and panel ingest |
| Parser location | Shared `gp16_sysex` lib; `gp16-dump` refactored onto it |
| Live parameter edit | Yes, final phase, gated on a hardware spike |
| Patch name editing | Local model only this pass — no name writes to the device |

Out of scope: write-to-internal-memory, drag-reorder of the chain, QML, custom knob painting,
theming, cloning `CircularSlider`.

**Every GUI phase must be verifiable without the GP-16.** The toolbar carries an *Open file*
action that feeds a saved `.bin` through the same `PatchBank` ingest as a live dump, so the
librarian, chain and forms can all be exercised against `captures/` and the committed
`midi-in.*.bin` / `device-dump.bin`. Only Phases 4 and 6 need hardware.

---

## Phase 0 — Correct the protocol docs (Completed)

Status: Completed

The old agent-doc "Key offsets (Temporary Buffer)" table was wrong and would mislead every later
phase. Corrections now live in `docs/GP16_PROTOCOL.md`. `midi_parameter_mapping.md` (Table 1)
is the source of truth; `GP16Editor.Models/Patch.cs` agrees with it. `GEMINI.md` is app/dev
guidance only (`CLAUDE.md` / `AGENTS.md` symlink to it).

| Former agent-doc claims | Actual offset |
|---|---|
| Compressor Sustain `06`, Attack `07` | Attack `0x10`, Sustain `0x11` |
| Distortion Drive `0B`, Turbo `0C` | Distortion Drive `0x14`; Overdrive Drive `0x17`; Turbo `0x18` (overdrive only) |
| Picking Filter Cutoff `11`, Up/Down `13` | `0x1B`, `0x1D` |
| Chorus Pre-Delay `23`, Rate `24`, Depth `25` | `0x33`, `0x34`, `0x35` |
| Reverb Time `3D`, Type `3F` | Decay `0x51`, Mode `0x52` |
| Device ID default `0x10` | `0x00` — matches both capture sets and `MainWindow.cpp:41` |

`06`/`07`/`0B`/`0C` are joint data and the B-2 mode byte. Writing live edits there would
scramble the effect chain, so this correction lands before any editor code.

**Files:** `docs/GP16_PROTOCOL.md`, `GEMINI.md` (plus `CLAUDE.md` / `AGENTS.md` → `GEMINI.md`)
**Done when:** protocol offsets match Table 1 and name `midi_parameter_mapping.md` as
authoritative.

---

## Phase 1 — Shared patch data layer

The single largest gap in the current tree. `MainWindow.cpp:186-210` only increments a byte
counter — there is no buffer, so "wire the dump into a list" is really "build accumulation,
slicing and parsing from scratch." Working versions already exist in
`qt/src/cli/gp16_dump.cpp`: `collectGroupPayload`, `takePatches` (`:537-551`) and the panel
indexing at `:462-467`. Lift all three into `gp16_sysex`, the static library both targets
already link, and put the CLI on top so the parser is proven against the captures before a
single widget exists.

Add `kPatchDataBytes = 117` to `RolandSysex.h` alongside the existing `kPatchSize = 0x7F`;
the two are different numbers and the distinction matters (see below).

### Two ingest shapes

Verified by parsing the committed captures:

| Path | Shape |
|---|---|
| RQ1 (`midi-in.20260913-*.bin`) | 34 DT1s per group: 33 × 245 data bytes + 1 × 107 = exactly 8192, contiguous, 7-bit carried addresses |
| Panel (`device-dump.bin`, `captures/dump-*.bin`) | 128 messages × 127 bytes, address `0F <idx> 00`, 117 data bytes, one patch per message |

Two consequences that the implementation must respect:

- **Group B answers at `02 00 00`, not the requested `01 40 00`.** Group A answers at
  `01 00 00` as expected. Do not validate response addresses against the request — accumulate
  in arrival order, which the captures show is gap-free.
- **Panel payload is 117 bytes (`0x00`–`0x74`), not 127.** It ends at `END OF PATCH NAME`.
  The name at `0x64`–`0x73` fits, but nothing past `0x74` exists on that path, while RQ1
  slices take `kPatchSize` = `0x7F`. Every field read must be length-guarded.

### Parsing rules

Follow `Patch.cs` offsets, with three corrections:

- **Do not port `Patch.cs:61-64`.** It reads `data[0..3]` as a verifiable / bulk-type /
  temp-memory / patch-number header that does not exist in a bulk slice — those bytes are
  JOINT DATA, so `PatchNumber` ends up holding an effect id in the range 0–4. Patch index
  comes from the buffer slot (RQ1) or `address[1]` (panel).
- **Count patches by stride, not by size.** `PatchService.cs:179` uses
  `buffer.Length / PATCH_SIZE` while slicing at `PATCH_OFFSET`; that is correct only by
  accident at 8192 bytes. Use `kPatchesPerGroup` and `kPatchStride`.
- Joint data per Table 1: offsets `00`–`04` are Block A order (a permutation of 0–4) with
  `05` **fixed at 5**; `06`–`0A` are Block B (6–10) with `0B` **fixed at 11**. Six slots per
  block, the sixth not reorderable.

The on/off bit map at `0x0D`/`0x0E`, the B-2 mode at `0x0C` and the A-2 distortion bit in
`Patch.cs:81-97` were checked against Table 1 and are correct — port them as they stand.

**Assemble MSB/LSB pairs as `(msb << 7) | lsb`.** Table 1's bit patterns confirm the widths:
Tap Delay taps are a 4-bit MSB plus a 7-bit LSB (11 bits, covering 0–1200ms), while Pitch
Shifter balance and the two cutoffs are a 1-bit MSB plus 7-bit LSB (covering 0–200). Note that
`Patch.cs` stores these as separate `…MSB`/`…LSB` properties and never combines them, so the
C# app almost certainly mis-renders those controls — do not treat it as a reference here.

### Offline verification

Add `gp16-dump --decode <file.bin>`: read a captured `.bin`, detect which ingest shape it is,
and print 128 rows of index, Roland ID, name, chain order and effect on/off flags. This is the
acceptance test for the phase and needs no hardware.

**Files:** add `qt/src/Patch.{h,cpp}`, `qt/src/PatchBank.{h,cpp}`; edit
`qt/src/cli/gp16_dump.cpp`, `qt/CMakeLists.txt` (new sources into the `gp16_sysex` target, not
the GUI target).
**Done when:** `gp16-dump --decode captures/dump-20260730-153932.bin` and
`--decode midi-in.20260913-082751.bin` both print 128 plausible patch names, and the existing
live dump modes still work unchanged.

---

## Phase 2 — Librarian window

Status: Completed

Chrome and list land together: a chrome-only step delivers nothing you can look at, and with
*Open file* in the toolbar this phase is fully verifiable with the unit unplugged.

Rewrite `MainWindow` as a `QMainWindow`: toolbar, splitter, status bar, and the existing log
moved into a bottom `QDockWidget`. Port combos, device ID spin (default `0x00`) and the dump
actions move from the central form into the toolbar. Default size ~1100×720.

Left pane: search field plus a `QListWidget` of 128 rows, `A11  Patch Name`. Roland ID helper:
index 0–63 is group A and 64–127 group B; bank = `(idx % 64) / 8 + 1`, number = `idx % 8 + 1`,
so index 0 is A11 and 127 is B88. Placeholder rows (`A11  —`) before any dump.

Three ingest actions, all landing in the same `PatchBank`:

- **Dump** — RQ1 Group A then B with the existing 50 ms gap. The state machine in
  `MainWindow.cpp:122-171` stays; it gains a payload buffer, then slices and refreshes the list.
- **Listen** — collect `0F <idx> 00` messages indexed by `address[1]`, with a progress count in
  the status bar and a manual stop, since the device decides when it is finished.
- **Open file** — feed a saved `.bin` through the same ingest, shape auto-detected.

Selecting a row shows ID and name in the header. The right pane may stay a placeholder in this
phase. Patch name is local-only — never sent to the device.

**Files:** `qt/src/MainWindow.{h,cpp}`, `qt/src/PatchListPanel.{h,cpp}`, `qt/README.md`
**Done when:** opening any committed capture fills 128 named rows; search filters live; the log
dock hides and restores; on hardware, Dump and Listen produce the same list as the file path.

---

## Phase 3 — Signal chain widget

Status: Completed

Two `QHBoxLayout` rows of checkable `QToolButton` chips. Click selects the slot; the checkbox
toggles the effect's on/off bit in the local model.

Chips are labelled by **effect name in joint-data order**, not by fixed position labels. "A-1"
is an effect identity (Compressor), not a chain position, so a reordered chain would display
wrong under static labels. Joint value → effect:

| Value | Effect | | Value | Effect |
|---|---|---|---|---|
| 0 | Compressor | | 6 | Short Delay |
| 1 | Distortion **or** Overdrive (bit 6 of `0x0D`) | | 7 | Chorus / Flanger / Pitch / Space-D (`0x0C & 0x03`) |
| 2 | Picking Filter | | 8 | Auto Panpot |
| 3 | Step Phaser | | 9 | Tap Delay |
| 4 | Parametric EQ | | 10 | Reverb |
| 5 | Noise Suppressor *(fixed last in Block A)* | | 11 | Lineout Filter *(fixed last in Block B)* |

**Enable checkboxes index by effect identity, not by chip position.** The chips are in joint
order but the on/off bits at `0x0D`/`0x0E` are in fixed identity order, so driving them from
the chip's position would toggle the wrong effect on any patch with a reordered chain.

Disabled slots stay visible and dimmed so the layout never jumps. No drag-reorder in this
pass — display order from the dump is enough.

**Files:** `qt/src/SignalChainWidget.{h,cpp}`, MainWindow wiring, CMake GUI target
**Done when:** two captured patches with different joint data show different chip sequences;
A-2/B-2 variant names match `0x0C`/`0x0D`; disabled chips stay visible and dimmed; clicking a
chip switches the (possibly empty) editor page.

---

## Phase 4 — SOUND CHANGE REQUEST probe (hardware gate)

Status: Completed

CLI-only, no GUI dependency — run it whenever the unit is at hand. It does not block Phase 5.

`midi_parameter_mapping.md` Table 2 documents offset `0x75` as SOUND CHANGE REQUEST, and the
manual's bypass example sends `00 00 0D …` **followed by** `00 00 75 …`.

**Finding (Play Mode, 2026-09-13):** temp-buffer DT1 alone is **not** audible; a following
SOUND CHANGE REQUEST at `00 00 75` **is required**. Compressor sustain at `00 00 11` without
`0x75` was silent; the same write plus `00 00 75` 50 ms later was heard. Device ID `0x00`.
Phase 6 must send `00 00 75` once after each coalesced burst settles.

Implemented as `gp16-dump --poke` (sustain high/low with and without `0x75`). Contract sentence
lives in `qt/README.md`.

**Files:** `qt/src/cli/gp16_dump.cpp`, `qt/README.md`
**Done when:** `qt/README.md` states whether Play Mode needs `0x75` for an audible change. ✓

---

## Phase 5 — Effect editor forms

Status: Completed

One `EffectEditor` over a `QStackedWidget`, data-driven from a spec table. Do not clone the 15
XAML views. Each slot is a list of parameter descriptors sourced from `midi_parameter_mapping.md`
Table 1.

Descriptor shape:

```
{ label, offset, byteWidth, min, max, type, displayTransform }
```

`byteWidth` is not optional. Pitch Shifter balance (`3B`/`3C`), all three Tap Delay taps
(`45`–`4A`), Tap Delay cutoff (`4F`/`50`) and Reverb cutoff (`53`/`54`) are MSB/LSB pairs; a
flat `{label, min, max, offset}` shape leaves those controls unreachable.

`displayTransform` covers the signed and scaled ranges — Compressor/Distortion/Overdrive tone
are `value − 50`, Pitch Shifter chromatic is `value − 12`, EQ levels are `0–48` shown as
−12…+12 dB, Q values are `0–40` shown as 1.0–5.0.

Types: slider + spin box, checkbox, combo. Special grouping for Parametric EQ (four band
columns) and Tap Delay (C/L/R tap rows). Read-only in this phase — forms display the selected
patch and update the local model; nothing is sent.

**Files:** `qt/src/EffectEditor.{h,cpp}`, `qt/src/EffectSpecs.{h,cpp}`
**Done when:** all 15 effects render, values from an opened capture match `--decode` output for
the same patch, and switching patches or slots updates the form with no stale values.

---

## Phase 6 — Live edit

Requires the Phase 4 result (**needs `0x75`** — see above). Do not omit the poke.

On control change: update the `Patch` model, then send via the existing
`MidiService::sendParameterChange` to `00 00 <offset>`.

`MidiService::sendBytes` (`MidiService.cpp:270`) sends immediately — the C# 50 ms inter-message
delay was never ported to Qt. The UI debounce is therefore the only rate limiter and is
load-bearing, not a nicety. Implement it as a per-address coalescing timer, last-value-wins,
~30–50 ms, so a slider drag emits one message per address per tick instead of flooding the
GP-16. Multi-byte parameters send MSB and LSB as one `buildDataSet` call with two data bytes,
coalesced as a single unit.

Send `00 00 75` **once** after a coalesced burst settles — not once per parameter message.

Skip the send when the output port is closed; the local edit still applies to the model.

**Files:** `qt/src/EffectEditor.{h,cpp}`, `qt/src/MainWindow.{h,cpp}`, possibly
`qt/src/MidiService.{h,cpp}` for the follow-up SOUND CHANGE REQUEST.
**Done when:** dragging a slider is audible on the device under the Phase 4 rule, the log shows
coalesced traffic rather than a flood, and closing the output port leaves editing functional
offline.

---

## Phase 7 — Docs

Update `qt/README.md`: new window description, the toolbar/dock layout, all three ingest paths
from the GUI, the `--decode` flag, and a corrected "Layout" section listing the new sources.
The Phase 4 hardware note is already there and stays.

---

## Verified facts backing this plan

Checked against the tree and the committed captures, not assumed:

- `MainWindow.cpp` is 258 lines and accumulates no payload — only `dumpPayload_` counters.
- `MidiService::sendParameterChange` exists at `MidiService.cpp:313` with zero callers.
- `MidiService::sendBytes` has no inter-message delay.
- RQ1 captures: 68 messages, 16384 payload bytes total, gap-free, Group B at `02 00 00`.
- Panel captures: 128 × 127-byte messages, 117 data bytes each, address `0F <idx> 00`.
- `Patch.cs` on/off bit map and offsets match `midi_parameter_mapping.md` Table 1 exactly;
  only the `data[0..3]` header read at `Patch.cs:61-64` is wrong.
- Device ID `0x00` in both capture sets.
- MSB/LSB widths from Table 1 bit patterns: 4+7 bits for Tap Delay taps (0–1200), 1+7 for
  Pitch Shifter balance and the Reverb / Tap cutoffs (0–200). `(msb << 7) | lsb` covers both.

Cross-checked against a second independent plan
(`~/.cursor/plans/qt_editor_layout_f44abe0e.plan.md`). The *Open file* action, the joint-value
table, the `(msb << 7) | lsb` rule, the standalone probe phase with a written-down result, and
the identity-vs-position warning on enable bits came from there.
