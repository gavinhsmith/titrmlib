# titrmlib — TI-84 Plus CE TUI Framework

Context for working on this codebase. This project is `titrmlib`, a TUI (text user
interface) framework for the TI-84 Plus CE calculator, built on `graphx`
(CE C toolchain / CEdev). It's a sibling project to Tincan and TINCLIB
(networking/Wi-Fi libraries for the same calculator over serial to an ESP
board) — `titrmlib` is meant to give those, and other CE apps, a way to build
calculator UIs out of composable TUI primitives instead of hand-rolled
`graphx` calls per screen.

## Core concept

`titrmlib` owns the entire display for a program, the way curses-style
libraries do on other platforms — not just a print-when-you-want-to output
helper. It renders the UI, reads keypad input, and drives the interaction
loop. An app built on `titrmlib` should never need to call `graphx` directly;
everything goes through `titrmlib`'s panel/widget API.

## Architecture

- **Cell grid core**: a character grid is the base rendering primitive.
  Grid dimensions (`term_cols`/`term_rows`) derive from the embedded font's
  glyph size and the screen size (320×240) at init.
- **Panel tree**: panels are not flat named regions (no fixed
  top/main/bottom) — they form a tree. A panel can split into children via
  something like `term_split(parent, direction, size)`, each child sized
  fixed, proportional to its parent, or filling remaining space. Layout is
  walked and computed per frame, so panels can appear/disappear based on
  app state without extra bookkeeping.
- **Panel-scoped output**: each panel owns its own cursor/print state,
  clipped to its bounds regardless of tree depth. `term_panel_print(panel,
  ...)` must never draw outside that panel's region.
- **Focus routing**: with a tree of panels, input needs an explicit focused
  panel (`term_focus(ctx, panel)`) and a traversal order between focusable
  children — there's no longer an implicit single interactive region.
- **Input**: keypad polling/reading is built into the library, not left to
  the app.
- **Render loop**: `titrmlib` drives draw + input each cycle, e.g.
  `term_run(update_fn, state)`, blocking until the app calls
  `term_quit(ctx, result)`. The framework owns "what's on screen right
  now," not the app.
- **Widgets (later phase)**: built as panel subtrees on top of the grid +
  tree + focus system — selectable lists, text input fields, scrollback,
  bordered panels.

## Font: fixed, compile-time, no FontLib

One bitmap font, chosen once, embedded at compile time. No runtime
font-switching, no FontLib dependency.

**Why not `graphx`'s native custom-font support
(`gfx_SetCharData`/`gfx_SetFontData`):** it's fixed at an 8×8 pixel cell
with 8px character advance, regardless of what's actually drawn inside the
cell. A narrower glyph just gets padded — `gfx_PrintString` still steps 8px
per character, so it does **not** buy the screen-real-estate win this
project wants. That ruled it out.

**Chosen approach:** a bespoke glyph-blit routine that bypasses
`gfx_PrintString` entirely, using a packed font data table and manual pixel
plotting (or run-based `gfx_HorizLine`/`gfx_FillRectangle` calls for
performance, preferred over per-pixel `gfx_SetPixel`).

- Source glyph data: [petabyt/font](https://github.com/petabyt/font)
  (`font.h`), MIT licensed. 5×7 bitmap font, uppercase/lowercase/digits/a
  decent ASCII punctuation set. Needs a one-off conversion script (not
  hand-transcription) from its `struct { char letter; char code[7][5]; }`
  array format into a packed byte table.
- Known gaps in that source to fill before use: no `|` glyph, one duplicate
  `-` entry, one glyph mapped to raw code `1` of unclear purpose — resolve
  these during conversion.
- Glyph size: **5×7**, +1px inter-character spacing and +1px line gap →
  6×8 cells, a **53×30** grid (vs. 40×30 at 8×8). `TERM_LINE_GAP 0` gives
  53×34, but capitals and digits then touch the line above. Legible in
  CEmu; still to be checked on a real calculator.
- Standard ASCII (0x20–0x7E) covers normal text.
- **0x80–0xFF reserved** for:
  - Box-drawing symbols (light set: `─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼`) for real
    panel borders, referenced by name (`TERM_CH_HLINE`, `TERM_CH_TL`,
    etc.) since they're outside ASCII.
  - Small status icons: checkmark/x-mark, connection/signal-strength
    indicators, selection/expand-collapse arrows, filled/empty dot — for
    device-picker / connection-status style screens (Tincan use case).
    Expect these to look crude at this cell size; verify on real
    hardware.
- Keep a code → glyph → purpose mapping table alongside the font source
  once built (not just in design discussion).

Sketch of the data format and blit routine (illustrative, not final):

```c
#define TERM_GLYPH_W 5
#define TERM_GLYPH_H 7

typedef struct {
    uint8_t rows[TERM_GLYPH_H]; // one byte per row, bits 4..0 = pixels L-to-R
} term_glyph_t;

extern const term_glyph_t term_font[256]; // generated, not hand-written

void term_draw_char(char c, int px, int py, uint8_t fg, uint8_t bg);
void term_draw_string(const char *s, int px, int py, uint8_t fg, uint8_t bg);
```

`term_panel_print` should call into this, using
`col * (TERM_GLYPH_W + 1)`, `row * TERM_GLYPH_H` for pixel positions —
not `graphx`'s built-in text calls.

**As built:** the blit (`draw_cell` in `src/titrm.c`) writes straight into
graphx's draw buffer (`gfx_vbuffer`), copying each cell row from a 64-entry
table of 6-pixel patterns. Run-based `gfx_HorizLine`/`gfx_FillRectangle`
calls measured about 1 ms per cell; the table copy is about 0.2 ms. The
table assumes the two fixed colors and has to change when color arrives.

## Build sequencing

All six phases exist; this is the order they were built in.

1. **Core cell grid**: init, cursor move, print. Keep this private/internal
   rather than the final public API — a curses-style event loop shouldn't
   require callers to reach into internals later.
2. **Font pipeline**: conversion script from `font.h` → packed
   `term_font[256]` table, plus the blit routine above.
3. **Input handling**: keypad reads wired in.
4. **Render loop**: `term` owns the draw/input cycle (`term_run`).
5. **Panel tree**: split/size semantics, focus routing.
6. **Widgets**: lists, input fields, borders (using the reserved
   box-drawing glyphs), and other composable pieces on top of the above.

## Decisions made

These were open questions; this is what the code does now.

- **Sizes:** `term_split(parent, dir, size)` with `TERM_FIXED(cells)`,
  `TERM_PERCENT(pct)`, `TERM_FILL` or `TERM_FILL_WEIGHT(w)`. Fixed and
  percent sizes are taken first; fill panels share the rest by weight, with
  the rounding remainder on the last one. All children of a panel use the
  same direction.
- **Layout** is recomputed only when the tree changes (split, destroy,
  show/hide, border), not every frame. Each panel's content rectangle is
  stored at layout time, because every character printed reads it.
- **Focus** moves in tree order (parents before children, siblings in split
  order, wrapping), skipping hidden and zero-sized panels. `[vars]` is
  `TERM_KEY_TAB` and moves focus; `term_focus()` sets it explicitly. There
  are no explicit next/prev links.
- **Event loop:** the app hands control to a blocking `term_run(ctx,
  update, state)` until `term_quit()`. The update callback gets key, tick,
  select and submit events. Panels are redrawn from scratch into a cell grid
  each frame, and only the cells that changed are drawn to the screen.
- **Input** is `os_GetCSC()`. It latches one key press between polls and
  repeats a held key, so keys are only lost if two arrive within a single
  frame. Keeping frames short is therefore what keeps input reliable (see
  Performance).
- **Color:** still deferred; see Non-goals.

## Testing

- **Host unit tests** (`tests/`, `make -C tests`): the library is compiled
  natively against stand-in CE headers (`tests/stubs/`). Drawing lands in a
  320×240 framebuffer that stands in for `gfx_vbuffer`, and the keypad
  replays scripted scan codes. CI runs them under gcc and clang with ASan
  and UBSan.
- **Hardware tests** (`tests/hw/`, `make hw-test`): small CE programs run in
  CEmu's `cemu-autotester`, which launches them, presses keys and compares
  CRCs of video memory with recorded screens. Nothing else drives the
  emulator. The developer supplies the autotester and a ROM with clibs
  installed: OS 5.4 or older, or 5.5+ jailbroken with arTIfiCE plus its
  AsmHook2 app (see `tests/hw/run.py`). Recording (`make hw-record`)
  replaces the CRC of titrmlib's own 8bpp screens and adds to the list for
  OS-drawn 16bpp screens; look at the PNGs before committing. CI only
  builds these programs, because running them needs a ROM.

## Performance

The eZ80 is slow in ways a desktop compiler hides. `int` is 24 bits,
multiplies by non-constants (`grid[r][c]` with a 53-cell row) and many
shifts become library calls (`__imulu`, `__bshl`), and every graphx call has
real overhead. Check `obj/<program>/lto.s` when a hot path is slow.

Emulated frame times (`tests/hw/perf`, which fails if they grow ~25%):

| frame | time |
|---|---|
| nothing changed, full screen of text rebuilt | ~76 ms |
| one row changed | ~105 ms |
| every cell changed | ~400 ms |

At the original ~280 ms per one-row change, two quick key presses could fall
into one frame and one was lost. The widgets hardware test now runs at the
autotester's default key speed to catch that coming back.

## Non-goals for this phase

- Color beyond whatever `graphx` defaults give you incidentally — no
  palette system, no per-cell fg/bg storage yet.
- Multiple simultaneous `titrmlib` instances sharing a screen with unrelated
  UI — one instance owns the whole screen for one program.
- FontLib, or any runtime font selection.
