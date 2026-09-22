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
- Target glyph size: **5×7**, +1px inter-character spacing → cell size
  6×7ish, screen grid roughly **53×34** cells (vs. 40×30 at 8×8). Confirm
  actual legibility on hardware/emulator once built.
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

## Build sequencing

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

## Open questions (not yet settled — use judgment, flag decisions made)

- Split/size constraint syntax for `term_split` — fixed cells,
  proportional (e.g. 30/70), fill-remaining — and whether layout
  recomputes every frame or only when the tree changes.
- Focus traversal order between sibling panels (tab order? explicit
  next/prev links?).
- Event loop shape: does the app hand control to `titrmlib` entirely
  (`term_run`), or does `titrmlib` expose a poll-style API the app calls each
  frame? Current lean is the former (blocking `term_run`).
- Color: deferred until the render loop, panel tree, and font are proven
  out. Non-goal for the initial build.

## Non-goals for this phase

- Color beyond whatever `graphx` defaults give you incidentally — no
  palette system, no per-cell fg/bg storage yet.
- Multiple simultaneous `titrmlib` instances sharing a screen with unrelated
  UI — one instance owns the whole screen for one program.
- FontLib, or any runtime font selection.
