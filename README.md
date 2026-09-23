# titrmlib
Terminal-Styled UI Framework for the TI-84 Plus CE

titrmlib owns the whole display: it draws a character grid with its own 5x7 font
(53x30 cells), reads the keypad and runs the event loop. An app builds a tree of
panels, gives them content, and calls `term_run()`. It never touches `graphx`.

```c
#include "titrm.h"

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) term_quit(ctx, 0);
}

int main(void) {
    term_ctx_t *ctx = term_init();

    term_panel_t *box = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "titrmlib");
    term_make_text(box, "Hello from titrmlib!");

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
```

## Layout

- `src/` - the library (`titrm.h` is the whole public API)
- `examples/hello/` - the smallest useful program
- `examples/demo/` - a mock Wi-Fi manager using every feature (panel tree, focus, list, input, log, progress, ticks, show/hide)
- `tools/gen_font.py` - converts `tools/petabyt-font/font.h` into `src/titrm_font.c`; also writes `src/titrm_chars.h` and `src/FONT.md` (the code -> glyph -> purpose table)
- `tests/` - host-side unit tests (see [Testing](#testing))
- `tests/hw/` - hardware tests run in CEmu's autotester (see [Hardware tests](#hardware-tests))
- `bin/` - build output (`.8xp` files), populated by `make`

## Building

Requires the [CE C/C++ Toolchain](https://github.com/CE-Programming/toolchain) (`CEdev`) on `PATH`.

```sh
make        # build every example into bin/<example>/
make demo   # or just one
make clean  # remove build artifacts
```

The library sources are compiled straight into each example (`project.mk`). The
project sits at the repo root on purpose: CEdev can't build sources reached
through `..` on Windows.

## Testing

```sh
make -C tests            # build and run the unit tests with the host C compiler
make -C tests CC=clang   # any C99 compiler; SANITIZE= disables ASan/UBSan
```

The tests don't need CEdev or a calculator. They compile the library natively
against stand-in CE headers (`tests/stubs/`): drawing lands in an in-memory
320x240 framebuffer and the keypad replays a scripted list of scan codes, so
layout, clipping, focus, widgets, key translation and the run loop are all
exercised through the public API.

### Hardware tests

The unit tests can't catch problems that only show up with the real compiler
(24-bit `int`), graphx, keypad and clock. For those, `tests/hw/` holds small CE
programs that run in [CEmu](https://github.com/CE-Programming/CEmu)'s
`cemu-autotester`. It launches each one, presses keys, and compares CRCs of
video memory with the expected screens. The tests use nothing else to drive or
inspect the emulator.

You supply the emulator and a ROM. The ROM isn't included, and CI can't run
these tests:

- `cemu-autotester` on `PATH` (CEdev ships it in `bin/`) or in `CEMU_AUTOTESTER`
- `AUTOTESTER_ROM`, a TI-84 Plus CE ROM image with the
  [CE C libraries](https://github.com/CE-Programming/libraries/releases) (`clibs.8xg`)
  already installed. Either:
  - **OS 5.4 or older**, where the autotester starts programs with `Asm(prgmNAME)`, or
  - **OS 5.5+ jailbroken with [arTIfiCE](https://yvantt.github.io/arTIfiCE/)**,
    with its `AsmHook2` app installed. `Asm(` is gone on these OS versions, so each
    test first runs AsmHook2 from the Apps menu (its hook doesn't survive the
    autotester's boot), then runs the program from the PRGM menu.

  The runner picks the launch method from the ROM: arTIfiCE if AsmHook2 is on
  it, `Asm(` otherwise. To force one, set `HW_LAUNCH=asm|artifice` or pass
  `--launch`. The menus are entered by letter (`[alpha][A]` for AsmHook2, then
  the program's first letter), so other apps and programs on the ROM are fine,
  unless one starts with the same letter and sorts first.

```sh
export AUTOTESTER_ROM=/path/to/ti84ce.rom
make hw-test                          # build and run all of them
make hw-test HW_ARGS="layout widgets" # or some of them
make hw-record                        # re-record the expected CRCs of failing screens
```

| Test | Checks |
|---|---|
| `canary` | Only the setup: a graphx program launches and exits. If this one fails, check the ROM and the libraries first |
| `glyphs` | Every character code, reverse video, box-drawing joins, word wrap |
| `layout` | Fixed, percent and weighted-fill sizes, 4-deep nesting, clipping, hide/show reflow, destroying a subtree |
| `widgets` | List, input and log driven by real key presses: wrap-around, `[enter]`, `[vars]` focus, alpha and alpha lock, `[del]`, scrollback |
| `ticks` | `term_set_tick` with the real `clock()`: 20 ticks of 100 ms arrive at plausible times |
| `perf` | Frame times for an unchanged screen, a one-row change and a full redraw stay within budget. A miss shows the measured time |
| `selfcheck` | Assertions that run on the calculator and read pixels back from VRAM: layout math, 24-bit `printf`, clipping, the log ring, focus, pool limits |

Every test ends by pressing `[clear]` and checking that the program returned to a
cleared home screen.

When a screen doesn't match, the runner saves it as a PNG in
`tests/hw/build/<test>/`. After an intentional rendering change, run
`make hw-record`. It rewrites the expected CRCs in each `autotest.json`, but look
at the PNGs before you commit them. Screens that titrmlib draws (8bpp) have
exactly one correct CRC, so recording replaces it. Home screen CRCs depend on
the OS version, so recording adds to that list instead.

### CI

CI (`.github/workflows/ci.yml`) runs on every push and pull request:

- **test** - the unit tests under gcc and clang with AddressSanitizer and UBSan
- **font** - reruns `tools/gen_font.py` and fails if the generated files in `src/` differ from what's committed
- **build** - builds every example and hardware test program with CEdev (after the tests pass). The hardware tests themselves need a ROM, so they don't run in CI

## Using it

**Panels form a tree.** `term_split(parent, dir, size)` adds a child; `dir` is
`TERM_HORIZONTAL` (side by side) or `TERM_VERTICAL` (stacked). Sizes are
`TERM_FIXED(cells)`, `TERM_PERCENT(pct)` or `TERM_FILL` / `TERM_FILL_WEIGHT(w)`.
Fixed and percent sizes are taken first and fill panels share the rest. Layout
is recomputed whenever the tree changes (split, destroy, show/hide, border).

**Everything is clipped to its panel.** `term_panel_print()` and friends can't
draw outside the panel's content area, at any depth. Panels are redrawn from
scratch each frame, so put custom output in a draw callback
(`term_panel_set_draw`).

**Widgets** turn a panel into something with built-in content and key handling:
`term_make_text`, `_list`, `_input`, `_log` (scrollback), `_progress`. Any panel
can also have a border and a title.

**Input.** Widgets get keys first. `[vars]` moves focus to the next focusable
panel in tree order. Keys nobody consumes reach your update function as
`TERM_EV_KEY`; widgets report `TERM_EV_SELECT` / `TERM_EV_SUBMIT`. `[alpha]` and
`[2nd][alpha]` (lock) type upper-case letters; `term_alpha_mode()` lets you show
the state. `term_set_tick(ctx, ms)` delivers `TERM_EV_TICK` for animation.

**Glyphs.** Codes 0x80-0xFF hold box-drawing characters and small icons
(`TERM_CH_TL`, `TERM_CH_CHECK`, `TERM_CH_SIG3`, ...). `TERM_S_*` are the same as
string literals: `TERM_S_CHECK "Done"`. See `src/FONT.md`.

## Limits

- One fixed font, no color (light on dark; `TERM_ATTR_REVERSE` inverts a cell).
- `TERM_MAX_PANELS` (32) panels; single-line input up to `TERM_INPUT_MAX` (48) characters.
- `TERM_LINE_GAP` (1) trades legibility for rows: 0 gives 34 rows, but capitals and digits then touch the line above.
