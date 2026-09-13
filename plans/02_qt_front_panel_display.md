# 02 — Qt front-panel display: Group / Bank·Number / LCD

Status: in progress (Phase 1–2 implemented against OM defaults; LCD line-2 packing still waiting on a Play Mode photo)
Scope: `qt/` tree only. The C# MAUI app is not touched.
Depends on: plan 01 Phases 1–3 (Patch, librarian, signal chain). Does not wait on Phase 7 docs.

## Photo — yes, please

A photo of **your** unit is the best reference. Not blocking — the owner’s manual already has the panel drawing (OM p.6) and Play Mode LCD rules (OM p.14) — but a picture locks three things the scan cannot:

1. **Straight-on 1U front**, display cluster only is enough (LCD + 2-digit patch display + Group LEDs). Used for proportions and colours.
2. **Close-up of the LCD in Play Mode**, one known patch (A11 is ideal). Used to lock the 16-character second line: off-symbol (`*` in the OM vs `>` in the OCR), and how Block A / Block B are packed into 16 columns.
3. Optional: 7-segment while a bank digit is blinking — not needed for this pass (display-only).

Shoot square-on, no heavy glare on the LCD. Phone photo is fine. Drop it in the chat (or into `docs/`) and the LCD formatter can be corrected before the widget is painted.

Until then, implement against the OM rules below and keep the line-2 packing in one function so a photo is a one-line change.

## Goal

The GP-16 display cluster is an **alternate header**, not a replacement. Both views stay in a `QStackedWidget`; **View → Front panel display** switches between:

- **Front panel** (default): Group indicator, 2-digit 7-segment Bank·Number, and the 16×2 Play Mode LCD
- **Name header**: Roland ID + patch name (`A11  Surfs Up Dude`)

This is **not** a full front-panel replica. No input jack, level LEDs, α-dial, number buttons, Edit/Write/System, or power switch. The rest of the window stays the plan-01 editor (toolbar, librarian, chain chips, effect form, MIDI log).

The cluster follows the **librarian selection**. It does not send MIDI, does not recall a patch on the device, and does not grow a second copy of the chain (the LCD second line is a 16-character summary; `SignalChainWidget` remains the navigation).

## What the hardware actually shows

1U rack (`482 × 44 × 300 mm`). Display cluster, left-of-centre on the face (OM p.6):

| Element | Hardware | This pass |
|---|---|---|
| Group indicator | LED: A yellow, B green (service notes `LN01401C` / `LN01301C`) | Two LED dots + A/B |
| Patch display | 7-segment, 2-digit (`SL-2263`). Bank digit + Number digit (`11`…`88`). Drawing shows `8.8` with a separator | Two digits, optional inter-digit dot |
| LCD | 16-letter × 2-line backlit character LCD (`DM045Z-7BL7`) | 16×2 Play Mode screen |
| Output channel LEDs | CH1 / CH2 | Out of scope |
| Input level LEDs | −40 / −20 / −10 / −5 / CLIP | Out of scope |

Play Mode LCD (OM p.14, example Group A / Bank 1 / Number 1):

- **Line 1** — patch name, 16 characters (offsets `0x64`–`0x73`). Pad with spaces; do **not** use trimmed `Patch::name()`.
- **Line 2** — signal-chain profile, left → right = joint-data order.
  - Block A then Block B.
  - Each slot is the **block-local number 1–6** when the effect is on, and `*` when off (OM p.14; confirm from photo).
  - A identities `0–5` → `1–6`; B identities `6–11` → `1–6`.
  - Off is **not** a blank; the slot stays visible so the line never jumps.

16-column packing, confirmed from a live Play Mode close-up (`11` / `Sparkling! AMP`):

```
A-AAAAAAB-BBBBBB
```

literal `A-`, six A chars, literal `B-`, six B chars. Example from that photo (A5+A6 on; B1, B2, B4, B5 on):

```
Sparkling! AMP
A-****56B-12*45*
```

Keep this packing in `playModeLcdLine2()` so a photo is a formatter tweak, not a widget rewrite.

Empty / no-patch state: 7-segment blank (or `--`), Group LEDs off, LCD both lines spaces (or a dim `NO PATCH` on line 1 — decide in implementation; prefer blank so it still looks like the unit at rest).

## Target layout (editor window)

```
QMainWindow
├── QToolBar          (unchanged)
├── QSplitter
│     ├── Left: librarian (unchanged)
│     └── Right
│           ├── PatchDisplayWidget     ← new, replaces headerId_/headerName_
│           │     [ A● B○ ]  [ 1.1 ]  [ 16×2 LCD ]
│           ├── SignalChainWidget      (unchanged)
│           └── EffectEditor           (unchanged)
├── MIDI log dock     (unchanged)
└── QStatusBar        (unchanged)
```

ASCII of the widget itself (1U strip, ~56–72 px tall, stretches horizontally):

```
┌─────────────────────────────────────────────────────────────┐
│  ○A  ●B     ╭─────╮     ┌──────────────────────────────┐    │
│  GROUP      │ 8.8 │     │ Sparkling    AMP             │    │
│             ╰─────╯     │ ****56  12*45*               │    │
│           Bank.Num      └──────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

Group A is yellow when `index < 64`, Group B green when `index >= 64`. Digits are `bank = (idx % 64) / 8 + 1`, `number = idx % 8 + 1` — same arithmetic as `Patch::displayIdFor`.

## Decisions

| Question | Decision |
|---|---|
| Full 1U faceplate (jacks, α-dial, 1–8, Edit/Write)? | **No.** Display cluster only. |
| Number buttons as a second librarian? | Later plan, not this one. |
| Send Program Change / temp-buffer recall on select? | **No.** Display follows the local librarian row. Live-edit still writes `00 00 xx` (plan 01 Phase 6). |
| QML / pixmap of the whole front? | **No.** `QWidget` + `QPainter`. A photo is a colour/layout reference, not a background image (it will not scale or stay readable). |
| Custom knobs / heavy QSS on the rest of the app? | **No.** Fusion stays everywhere except this strip. |
| LCD font | 5×7 pixel characters on a grid (HD44780-style). Not a proportional UI font, not a 7-seg font for the name. |
| 7-segment | `QPainter` segments, not a webfont. Green LEDs (the unit is green, not red). |
| Palette | Dark grey bezel; green 7-seg; A LED amber, B LED green; LCD is dark characters on a lime-green backlight (STN), not lit pixels on black. |
| Offline verification | Yes — *Open file* + captures, same as plan 01. |

Out of scope: write-to-internal-memory, patch recall on the device, Edit Mode LCD pages (parameter names, `>` cursor), blinking bank digit, output-channel LEDs, LCD contrast control.

## Why not a photo widget

A `QLabel` with a PNG of the front panel looks like a screenshot, breaks at HiDPI, and cannot show 128 different names. Paint the three instruments the unit actually has (LEDs, 7-seg, character LCD). That is what makes it read as a GP-16 rather than a themed header.

## Implementation

### Play Mode strings (no widgets)

Add to `gp16_sysex` so the CLI and tests share them:

```
// Patch.h / Patch.cpp or a tiny LcdPlayMode.{h,cpp} in gp16_sysex
char groupLetterFor(int index);          // 'A' / 'B'
int bankDigitFor(int index);             // 1–8
int numberDigitFor(int index);           // 1–8
std::string playModeLcdLine1(const Patch&); // exactly 16 chars
std::string playModeLcdLine2(const Patch&); // exactly 16 chars
```

`displayIdFor` stays (`"A11"`) for the librarian list. The new helpers are the split the 7-seg and Group LEDs need.

Line 1: 16 bytes at `0x64`, space-padded, ASCII 32–126 (same rules as `extractPatchName` but **no trim**). Missing patch → 16 spaces.

Line 2: `A-` + six Block A chars + `B-` + six Block B chars. Each slot is `'1'+local` if `isEffectEnabled`, else `'*'`. `local` is `identity` for A, `identity - 6` for B. Confirmed on a live unit close-up (`A-****56B-12*45*`).

`gp16-dump --decode` can grow an optional LCD column later; not required to ship the widget.

### Widget

`qt/src/PatchDisplayWidget.{h,cpp}` — one `QWidget`, `setPatch(const Patch*)`, public getters for tests:

```
groupLetter / bankDigit / numberDigit / lcdLine1 / lcdLine2
```

Paint in `paintEvent`. Do not build this out of `QLabel` + stylesheet 7-seg tricks; the LCD grid and 7-seg need `QPainter`.

- Minimum height ~64 px; horizontal stretch; no vertical stretch (the effect form keeps the leftover height).
- High-DPI: draw in device pixels (`devicePixelRatioF()`), integer LCD cell sizes so the 5×7 dots stay sharp.
- `QSizePolicy::Preferred / Fixed` (height).
- Empty patch: blank digits, both LEDs off, LCD spaces.

### MainWindow

In `MainWindow::` constructor, drop `headerId_` / `headerName_` / the header `QHBoxLayout`. Insert `PatchDisplayWidget` where the header was.

`updateHeader(int)` becomes `display_->setPatch(index valid ? &bank_.patchAt(index) : nullptr)`.

Chain-toggle already mutates `Patch::setEffectEnabled`; call `display_->setPatch(...)` (or a `refresh()`) from `onChainEffectToggled` so LCD line 2 tracks enable bits without waiting for a row change.

### Tests (no hardware)

`qt/tests/test_patch_display.cpp`:

- `bankDigitFor(0)==1`, `numberDigitFor(0)==1`, `groupLetterFor(0)=='A'`
- index 127 → B / 8 / 8
- Open `captures/dump-20260730-153932.bin` patch 0: line 1 equals the 16-char name field; line 2 length 16; on-slots are digits, off-slots are `*`
- Two patches with different joint data produce different line 2 (same bar as the signal-chain test)
- Widget: `setPatch` then getters match the free functions
- Main window: after *Open file*, the display widget’s digits are `1`/`1` and group `A` for row 0 (extend `test_main_window.cpp` if cheaper than a new binary)

Wire the new test target in `qt/CMakeLists.txt` like the other widget tests.

## Phases

### Phase 1 — LCD / ID helpers + tests

**Files:** `qt/src/Patch.{h,cpp}` (or `LcdPlayMode.{h,cpp}` into `gp16_sysex`), `qt/tests/test_patch_display.cpp`, `qt/CMakeLists.txt`

**Done when:** `gp16_patch_display_tests` (or the sysex test binary) prints 16-char lines for both committed captures; joint-data order and on/off `*` match `Patch::blockAOrder` / `isEffectEnabled`. No widgets yet.

If a photo arrives during this phase, fix `playModeLcdLine2` here.

### Phase 2 — Widget + MainWindow

**Files:** `qt/src/PatchDisplayWidget.{h,cpp}`, `qt/src/MainWindow.{h,cpp}`, CMake GUI target, `qt/tests/test_main_window.cpp` (or the widget test)

**Done when:** opening any committed capture shows Group A, `1.1`, and the 16×2 name/chain for A11; selecting B88 updates Group B and `8.8`; toggling a chain chip changes LCD line 2; no patch selected blanks the cluster; librarian list still shows `A11  Name`.

## Visual bar (not a theme pass)

Spend the character on the **LCD**. 5×7 dots, green-yellow on a very dark green well, slight inset bezel, 16 columns × 2 rows with a one-cell gap. 7-seg is supporting: chunky, red, two digits with a dim colon/dot. Group LEDs are 6–8 px circles, only one lit. Everything around them is matte black with a thin grey hairline — the 1U extrusion, not a drop-shadow card.

Do not: cream/terracotta palette, acid-green-on-near-black synth skin on the whole window, rounded SaaS cards, or a photographed aluminum texture.

## Relation to plan 01

Plan 01 Phase 2 shipped `headerId_` + `headerName_` as a placeholder. This plan **replaces** that placeholder. Phase 3 chain chips and Phase 5–6 editors stay. Phase 7 README should mention the display cluster when that docs pass happens.

## Verified facts

- OM p.6 front-panel drawing: Patch Display is the 2-digit 7-seg; Display is the LCD to its right; Group Indicator is separate and to the left of the 7-seg.
- OM p.14: Play Mode LCD is name (16) + Block A/B numbers; on = number, off = `*`; order is connection order left → right; A 1–6 / B 1–6 as listed.
- Specs: “16-letter, 2-line LCD (Back lit.)”, “Patch display (7-segment, 2-digit)”.
- Service notes: Group A LED yellow, Group B green; 7-seg `SL-2263`; LCD `DM045Z-7BL7`.
- `Patch::displayIdFor` already encodes Group/Bank/Number; `extractPatchName` **trims** and is the wrong source for LCD line 1.
- Current header is two `QLabel`s in `MainWindow.cpp`. No tests assert on those labels; librarian tests assert on list rows (`A11  …`) and must keep doing so.
