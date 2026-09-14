# 03 — Import legacy `.PCH` patch charts into the GP-16

Status: in progress (phases 0–2 done)
Scope: `qt/` tree only. The C# MAUI app is not touched.
Depends on: plan 01 (`Patch`, `PatchBank`, `EffectSpecs`, `MidiService`, `MainWindow` chain/editor —
all already implemented and reused here, nothing rebuilt from scratch).

## Goal

`patches/*.PCH` are hand-typed patch charts — plain text, one per patch, following the blank
template in `patches/GP_16PCH.TXT`. The format dates back to 1994 (first-year-of-uni vintage,
predates any of this tooling by three decades), so it is a human documentation aid, not a
machine-verified export — no automated tool has ever guaranteed a `.PCH` file agrees with what
was actually programmed into the unit at the time.

Target flow: **Import Patch…** → pick a `.PCH` file → parser produces a settings overview →
user picks a destination bank/patch (e.g. `B21`) → the parsed patch is applied there and sent
to the GP-16.

## What already exists (reuse, don't rebuild)

- `Patch` — raw 117-byte buffer, byte/word accessors, block-order arrays, on/off bitmask,
  `displayId()` (`A11`…`B88`).
- `EffectSpecs` — per-effect `ParamSpec` tables for offsets `0x0F`–`0x5A`, plus
  `rawToDisplay` / `displayToRaw` for every transform the charts use (dB, Q, linear/log Hz,
  offset-50, offset-12). This is most of the unit-conversion work already done.
- `PatchBank` / `PatchListPanel` — the 128-slot librarian, reusable as the destination picker.
- `MidiService::sendParameterChange` + `MainWindow`'s coalesced edit path — the only
  **proven-on-hardware** write path today (temporary buffer, `00 00 <offset>`, followed by a
  Sound Change Request at `0x75`). Writing directly into internal memory at a chosen bank/patch
  address has never been attempted by this codebase — see Phase 4.
- `gp16-dump --decode` (CLI) — existing pattern for printing a human-readable patch overview
  from raw bytes; Phase 2 below reuses that idea for the text-import path.

## Format quirks worth knowing about (why Phase 1 is its own phase)

Scanned all 13 sample files in `patches/`. The template is sparse: a block's section is only
printed when that block is *on*, per `BLOCK A/B - ON/OFF` (see `Patch::playModeLcdLine2`, which
already implements the same digit/`*` encoding this parser has to invert). Beyond that:

- Whitespace is inconsistent — tabs vs. spaces vs. none, even between sibling lines in the same
  file (`A-2  \ta. DISTORTION` vs. `A-2  b. OVERDRIVE` in `EDGEECHO.PCH`). One file
  (`BTT70S.PCH`) has a bare `----------------------------------` separator with no leading space
  where every other file and every other separator in that same file has one.
- Numbers carry units inline and formatting varies: `8.00 kHz` vs `8.0 kHz`, signed dB values
  (`+11.0 dB`, `-3.0 dB`, `0.0 dB`), `CUTOFF THRU` as a sentinel for the filter's bypass position
  rather than a number.
- Reverb mode is spread across four `MODE: <FAMILY>` lines with only one populated
  (`MODE: SPRING 2`) — needs combining into the single 0–9 combo index `EffectSpecs` expects.
- `SEQUENCE BLOCK A/B` is usually the default `123456` but not always (`BTT70S.PCH` has
  `132546`) — the effect chain order has to be read from this line, not assumed.
- The `BLOCK A/B - ON/OFF` summary line and which sections are actually printed can disagree.
  In `BTT70S.PCH`, block 4 (Step Phaser) gets a header with blank values though flagged off —
  consistent — but the on/off row also flags block 6 (Noise Suppressor) as on, and no A-6
  section is printed at all. That is very likely a small slip made by hand thirty years ago, not
  a format rule. The parser should trust *populated values* over the summary row and log the
  disagreement as a warning rather than silently "fixing" it one way or the other.
- `Author` / `Comments` / `PROGRAM CHANGE NO.` are documentation only — the GP-16 patch buffer
  has no fields for them (only a 16-character name at `0x64`–`0x73`, per
  `docs/GP16_PROTOCOL.md`). Show them in the preview; never write them.

None of this is a knock on the format — it was never meant to be machine-read. It just means
the parser has to be forgiving by design, and every skipped or guessed line needs to be visible
to the user, not swallowed.

## Decisions to take up front

| Question | Decision |
|---|---|
| Where do Author/Comments/Program Change go? | Preview only; never written to the device (no field exists for them) |
| Scope of parameters in v1 | Core per-effect params (existing `EffectSpecs` coverage) + Master Volume + Output Channel. Expression pedal assign/device/LFO/max-min (Table 4, only used by 1 of 13 samples) deferred to Phase 5 |
| How to resolve on/off vs. printed-section disagreements | Trust which sections have parsed values; log disagreements as warnings, never guess silently |
| How to write into a chosen bank/patch slot | Ship the proven temp-buffer load path first (Phase 4a); attempt a direct internal-memory DT1 write behind a hardware spike (Phase 4b) since it has never been tried in this codebase |

---

## Phase 0 — Global (non-effect) parameter coverage

Status: done

`EffectSpecs` only covers offsets `0x0F`–`0x5A` (the 16 per-effect blocks). Master Volume
(`0x5B`) and Output Channel (`0x63`) are needed for every import and currently have no
`ParamSpec`-shaped home anywhere in the Qt tree — nothing in the live editor exposes them
either, which is a pre-existing gap this phase happens to close. Add them (flat list is fine,
doesn't need the full `EffectSpec`/`LayoutKind` machinery built for per-slot forms) and record
the offsets in `docs/GP16_PROTOCOL.md`'s "Key Temporary Buffer offsets" table alongside the
per-effect highlights already there (full detail already exists in
`midi_parameter_mapping.md` Table 1, rows `5B`/`63`).

**Files:** `qt/src/EffectSpecs.h`/`.cpp` (or a small new `GlobalParams.{h,cpp}` if that reads
cleaner), `docs/GP16_PROTOCOL.md`
**Done when:** `MASTER VOLUME 55` / `CHANNEL 1` style lines from any sample `.PCH` round-trip
through `displayToRaw`/`rawToDisplay` to the values Table 1 predicts.

---

## Phase 1 — `.PCH` chart parser (no MIDI, no UI)

Status: done

A lenient, line-oriented parser — new `PatchChartParser` in the shared `gp16_sysex` static lib
(same home as `RolandSysex`, linked by both GUI and CLI). Input: chart text. Output: a
`ParsedChart` struct — name, author, comment, program-change number (doc fields, kept for the
preview only), Block A/B order arrays, per-slot presence + raw field values keyed by section
header, the global fields from Phase 0, and a list of warnings (unparsed lines, values outside
`ParamSpec` range, the on/off-vs-printed-section disagreement called out above).

Match section headers (`A-1`…`A-6`, `B-1`…`B-6`, and the `a./b./c./d.` sub-variants for `A-2`
and `B-2`) and `LABEL value [unit]` lines by tokenizing rather than exact string match, so tab
vs. space differences and stray separators don't break parsing. Convert `kHz`→Hz, strip sign
and unit before parsing dB, and map `CUTOFF THRU` to the "bypass" end of that parameter's range
per `EffectSpecs`. Combine the four `MODE: <FAMILY>` reverb lines into one combo index using the
existing `kReverbMode` ordering.

**Files:** `qt/src/PatchChartParser.{h,cpp}` (new), `qt/CMakeLists.txt`
**Done when:** all 13 files in `patches/*.PCH` parse without crashing, and a manual side-by-side
of `ACOUSTIC`, `KNOPFLER`, `BTT70S` and `SHADOWS` against their source text confirms values
match — including the two known `BTT70S.PCH` inconsistencies surfacing as warnings, not as
silently "corrected" data.

---

## Phase 2 — Chart → `Patch` conversion + CLI verification

Status: done

Convert a `ParsedChart` into a real `Patch` byte buffer: `EffectSpecs::writeParam`/
`displayToRaw` for per-effect fields, `Patch::setEffectEnabled` for the on/off bitmask, direct
byte writes for the block-order arrays and the Phase 0 global fields, ASCII into the name field.
Add `gp16-dump --import <file.pch> --decode` to the existing CLI so the resulting `Patch` prints
through the same decode path already used for captured dumps — lets the human compare a
`.PCH` file's own text against what the importer understood, without touching the GUI or the
device, matching this project's "every phase verifiable without hardware" convention.

**Files:** `qt/src/cli/gp16_dump.cpp`, `qt/src/PatchChartParser.{h,cpp}` (conversion function)
**Done when:** `gp16-dump --import patches/<name>.PCH --decode` matches the source chart by eye
for all 13 sample files.

---

## Phase 3 — Qt "Import Patch…" flow (UI, offline)

Status: not started

Toolbar/menu action → `QFileDialog` (default directory `patches/` if present, otherwise the
existing `lastOpenDir_`-style remembered directory) → parse → preview dialog (effect list with
on/off + key values, name/author/comment shown as metadata, warnings list surfaced — not just
logged) → destination picker reusing `PatchListPanel`'s 128-slot list, labelled with
`displayId()` (`B21` etc.), defaulting to the currently selected librarian slot → **Import**
applies the converted `Patch` into `bank_.patchAt(destinationIndex)` and refreshes the
librarian/chain/effect editor. No MIDI in this phase — fully testable against all 13 sample
files with no device connected, same as the existing `.bin`-file open path.

**Files:** `qt/src/PatchImportDialog.{h,cpp}` (new), `qt/src/MainWindow.{h,cpp}`,
`qt/CMakeLists.txt`
**Done when:** importing any sample `.PCH` with no device connected updates the librarian entry
at the chosen slot, and the effect editor / front-panel display reflect it, matching Phase 2's
CLI output for the same file.

---

## Phase 4 — Send to the GP-16 (hardware gate)

Status: not started

Two steps, same shape as plan 01's Phase 4/6 precedent: ship the already-proven path first,
gate the exploratory one behind hardware verification.

**4a — Load to Temporary Buffer.** Reuse the existing coalesced-DT1 + Sound Change Request path
(`MainWindow::onParameterEdited` / `onEditCoalesceTick`) to push every byte the import changed,
so the patch is immediately audible. Tell the user (status bar / dialog) that saving it into the
chosen bank/patch slot still needs **WRITE** on the device front panel — this matches what
`docs/GP16_PROTOCOL.md` already documents about the temporary buffer.

**4b — Direct internal-memory write (spike).** Table 1's addressing rules imply a DT1 to
address `01 <patch index> 00` with the full patch payload should write straight into that
bank/patch slot — the same address family already used for the internal bulk-dump *read*
(`kInternalGroupAAddress`/`kInternalGroupBAddress`) — but no message like it has ever been sent
by this codebase, and the manual's one worked example around this area (Table 1, footnote near
row `63`) reads oddly enough (`10` for what should be a set/request command) that it can't be
trusted at face value; verify against real hardware before relying on it. If it works, make it
the primary "Write directly to bank/patch" flow with 4a kept as an audition step; if not, ship
4a only and record the finding.

**Files:** `qt/src/MainWindow.{h,cpp}`, `qt/src/MidiService.{h,cpp}` (only if a payload-sized
send helper beyond the existing `sendParameterChange` span overload turns out to be needed),
`qt/README.md`, `docs/GP16_PROTOCOL.md`
**Done when:** `qt/README.md` states definitively whether a direct internal-memory patch write
works, and at least one imported sample patch has been confirmed correct by ear on real
hardware.

---

## Phase 5 — Export a patch to `.PCH`

Status: not started

The inverse of Phases 1–2: given a `Patch`, render a chart in the same format as
`patches/GP_16PCH.TXT`, so a patch fetched from the device (or edited live) can be written back
out as a chart for safekeeping — the whole reason the samples in `patches/` exist in the first
place. New `PatchChartParser::toChartText(const Patch&, const ChartMetadata&)` (or a sibling
`PatchChartWriter`) walks the same section list as the parser, using `EffectSpecs::rawToDisplay`
for every field and `Patch::isEffectEnabled`/block-order to decide which sections to print —
only on sections get a header, matching the samples' own convention (no need to reproduce their
whitespace inconsistencies; one clean, consistent layout for everything this tool writes).

`Author` / `Comments` / `PROGRAM CHANGE NO.` have no home in the device data (Phase 1's finding),
so they can't be recovered from a `Patch` alone — prompt for them in the export dialog (optional,
blank is fine) rather than silently omitting the header lines the format expects.

UI: **Export Patch…** alongside **Import Patch…**, acting on the librarian's current selection;
`QFileDialog` save, default directory `patches/` if present. CLI: `gp16-dump --export <bank/patch
or file.bin> --out file.pch` for the same offline-testable verification path used elsewhere in
this plan — round-trip `--import file.pch --export --out roundtrip.pch` and diff against the
original for every field the format can represent (the closest thing to a regression test this
project has, given there's no test suite).

**Files:** `qt/src/PatchChartParser.{h,cpp}` (writer function), `qt/src/PatchExportDialog.{h,cpp}`
(new, or fold into `PatchImportDialog` if the two end up sharing enough UI), `qt/src/MainWindow.{h,cpp}`,
`qt/src/cli/gp16_dump.cpp`
**Done when:** exporting any of the 13 sample patches (once importable) and re-importing the
result reproduces the same `Patch` bytes, and a fetched-from-hardware patch exports to a chart
that reads correctly by eye.

---

## Phase 6 — Polish (optional, not blocking)

Status: not started

- Expression pedal assign/device/LFO/max-min (Table 4) support, deferred from Phase 0 since only
  `BTT70S.PCH` among the samples uses it. Needed on both the import and export sides once added.
- Promote warnings out of the log dock into something more visible in the preview dialog
  (icon/tooltip) once real usage shows the log alone gets missed.
