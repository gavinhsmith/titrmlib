# titrmlib

[![CI](https://github.com/gavinhsmith/titrmlib/actions/workflows/ci.yml/badge.svg)](https://github.com/gavinhsmith/titrmlib/actions/workflows/ci.yml)
[![API docs](https://img.shields.io/badge/docs-API%20reference-blue)](https://gavinhsmith.github.io/titrmlib/docs/api.html)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue)](https://github.com/gavinhsmith/titrmlib/blob/main/LICENSE.md)
[![Platform: TI-84 Plus CE](https://img.shields.io/badge/platform-TI--84%20Plus%20CE-lightgrey)](https://github.com/CE-Programming/toolchain)

A terminal-style UI framework for the TI-84 Plus CE.

<img src="demo.gif" alt="The demo program: a mock Wi-Fi manager with a network list, details, log, command input and status bar" width="640">

titrmlib takes over the calculator's screen the way curses does on a desktop
terminal. It draws a 53×30 character grid with its own 5×7 font, reads the
keypad, and runs the event loop. You build the UI as a tree of panels, give
them content, and hand control to `term_run()`. Your program never calls
graphx itself.

- **Panel tree:** split any panel into fixed, percentage or weighted-fill children, nested as deep as you need
- **Scenes:** several full-screen panel trees, one shown at a time, each with its own event handler
- **Overlays:** panels drawn on top of a scene, such as dialogs, that give focus back when they close
- **Clipped output:** each panel has its own cursor, and nothing drawn in it can spill outside it
- **Focus:** your app decides which panel has focus; the focused widget gets keys first
- **Widgets:** text (with scrolling and auto-scroll, for logs), list, input, button, checkbox, progress bar, plus borders and titles; build your own with a key handler
- **Color:** foreground and background per panel, from the 256-color palette; reverse video swaps them
- **Text styles:** bold, italic, underline and strikethrough, per panel or inline in a string, and tab stops
- **Glyphs:** box-drawing characters and small status icons (check marks, signal bars, arrows) alongside ASCII
- **Ticks:** timed events for animation and polling

## Example

```c
#include "titrm.h"

/* The event handler: return true for events it handled. */
static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
        return true;
    }
    return false;
}

int main(void) {
    term_ctx_t *ctx = term_init();

    term_panel_t *box = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "titrmlib");
    term_make_text(box, "Hello from titrmlib! Press [clear] to quit.");

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
```

`examples/demo/` is a larger program: a mock Wi-Fi manager that uses every
feature.

## Requirements

- To build: the [CE C/C++ Toolchain](https://github.com/CE-Programming/toolchain) (CEdev).
- To run: a TI-84 Plus CE with the
  [CE C libraries](https://github.com/CE-Programming/libraries/releases)
  installed, and an OS that can run C programs: 5.4 or older, or a newer OS
  jailbroken with [arTIfiCE](https://yvantt.github.io/arTIfiCE/).

## Adding it to your project

titrmlib is compiled into your program from source. Put the repository inside
your CEdev project, for example as a git submodule at `lib/titrmlib`, and add
its sources to your makefile:

```make
CFLAGS = -Wall -Wextra -Oz -Ilib/titrmlib/src
EXTRA_C_SOURCES = $(wildcard lib/titrmlib/src/*.c)
```

Keep it inside the project directory. On Windows, CEdev can't build sources
reached through `..`.

## Using it

The whole API is in [`src/titrm.h`](src/titrm.h); the [API reference](docs/api.md) lists every function, type and constant.

**Panels.** `term_split(parent, dir, size)` adds a child to a panel. `dir` is
`TERM_HORIZONTAL` (side by side) or `TERM_VERTICAL` (stacked), and every child
of a panel uses the same one. Sizes are `TERM_FIXED(cells)`,
`TERM_PERCENT(pct)`, `TERM_FILL` or `TERM_FILL_WEIGHT(w)`. Fixed and percent
sizes are taken first, then fill panels share what's left. Panels can be
hidden (`term_panel_show`) or removed (`term_panel_destroy`), and their
siblings reflow.

**Drawing.** Output is retained, like curses: what you print into a panel
with `term_panel_print`, `_printf`, `_putc`, `_move` and `_set_attr` stays
there until you overwrite it or call `term_panel_clear`. Print whenever your
state changes, typically in the update function; only the cells that changed
are redrawn. Output is clipped to the panel's content area, and only panels
without children hold content.

**Color.** `term_panel_set_colors(panel, fg, bg)` takes palette indices:
`TERM_COLOR_*` names common ones in graphx's default palette, and any index
from 0 to 255 works. Like the attribute, colors apply to what is printed next
and to `term_panel_clear`; widgets, borders and titles are drawn in them.
Panels split from a panel start with its colors, so coloring a dialog colors
everything in it. `TERM_ATTR_REVERSE` swaps a panel's two colors. The default
is white on black.

**Styles.** `TERM_ATTR_BOLD`, `TERM_ATTR_ITALIC`, `TERM_ATTR_UNDERLINE` and
`TERM_ATTR_STRIKE` combine with each other and with `TERM_ATTR_REVERSE`, e.g.
`term_panel_set_attr(p, TERM_ATTR_BOLD | TERM_ATTR_UNDERLINE)`. To style part
of a string, put `TERM_S_BOLD`, `TERM_S_NORMAL` and the like inside it:
`"press " TERM_S_BOLD "enter" TERM_S_NORMAL " to connect"`. Printed, they set
the panel's attribute; in a text widget, list item or label they last until
the next one or the end of the line. Underline and strikethrough join across
cells; box-drawing characters ignore styles. `'\t'` moves to the next tab
stop, every 4 columns.

**Widgets** turn a panel into one with built-in content and key handling:
`term_make_text`, `term_make_button`, `term_make_checkbox`, `term_make_list`,
`term_make_input` and `term_make_progress`. A text widget copies its text and
can grow with `term_text_append`; with `term_text_autoscroll` it follows the
end, which makes it a log. A panel that holds other panels is a container:
give it a border and title with `term_panel_set_border` and
`term_panel_set_title`.

Widgets draw in the panel's attribute (`term_panel_set_attr`) and colors,
and show focus with its focus attribute (`term_panel_set_focus_attr`); text
can be centered
with `term_panel_set_align`. To build your own widget, make a panel
focusable, give it a key handler with `term_panel_set_keys`, print its
content, and report changes with `term_panel_send(panel, TERM_EV_CHANGE,
value)`.

**Scenes.** `term_root(ctx)` is the first scene. `term_scene_new(ctx,
handler, state)` creates another full-screen scene, and `term_scene_switch`
shows it. Only the active scene is drawn and gets events; the others keep
their content until you switch back. A scene's handler hears
`TERM_EV_SCENE_ENTER` and `TERM_EV_SCENE_LEAVE` when it's switched to or away
from.

**Overlays.** `term_overlay_open(ctx, col, row, w, h)` or
`term_overlay_open_centered(ctx, w, h)` returns a panel drawn on top of the
active scene; split it and fill it like any panel. `term_overlay_close()`
removes it and gives focus back to the panel that had it when the overlay
opened, if focus was inside the overlay or empty. Overlays aren't modal: your
app can move focus between an overlay and what's under it.

**Events.** Handlers receive:

- `TERM_EV_START`, once before the first frame
- `TERM_EV_KEY`, for keys the focused widget didn't use
- `TERM_EV_SUBMIT`, when the user confirms with `[enter]`: an input, a button, or a list item (`value` is its index)
- `TERM_EV_CHANGE`, when the user changes a widget: moves a list selection, edits an input, or toggles a checkbox
- `TERM_EV_FOCUS_LOST`, when the focused panel is hidden or destroyed
- `TERM_EV_TICK`, every interval set with `term_set_tick(ctx, ms)`

An event goes to the active scene's handler first, then to the global handler
passed to `term_run()`. A handler returns `true` when it has handled the event,
which stops it there.

**Focus.** Your app moves focus with `term_focus(ctx, panel)`; nothing is
focused until it does, and titrmlib never moves it on its own. `[vars]` is an
ordinary key (`TERM_KEY_VARS`), so you can use it to cycle focus, as the demo
does. Switching scenes clears focus if it was on the old scene. Input, list,
button and checkbox widgets are focusable; other panels, such as a text log
that should scroll, opt in with `term_panel_set_focusable`.

**Keys.** `[alpha]` types one upper-case letter and `[2nd][alpha]` locks
alpha; `[alpha]` itself arrives as `TERM_KEY_ALPHA`, and `term_alpha_mode()`
reports the state. Held arrow keys and `[del]` repeat. `term_quit(ctx,
result)` ends `term_run()`, which returns `result`.

**Special characters.** Codes outside printable ASCII follow code page 437,
the IBM PC character set: box drawing (single and double lines), blocks and
shades, accented letters, Greek and math symbols. The exception is 0x13–0x16,
which hold signal-strength icons. The characters titrmlib uses are named
`TERM_CH_*` (e.g. `TERM_CH_SIG3`), and the same characters as string literals
are `TERM_S_*`: `TERM_S_SIG3 " HomeWiFi"`. Any other code can go straight into
a string (`"caf\x82"` prints `café`). 0x00, `\n` (0x0A) and `\r` (0x0D) can't
be printed from a string, so place those glyphs with `term_put`. The full table
is in [`src/FONT.md`](src/FONT.md).

## Limits

- One built-in font.
- Screens using many color pairs at once draw more slowly: the pixel tables for
  the six most recent pairs are cached.
- Up to `TERM_MAX_PANELS` panels, and `TERM_INPUT_MAX` (48) characters in an
  input field. The pool holds 32 panels by default, at about 80 bytes of RAM
  each; build with e.g. `-DTERM_MAX_PANELS=12` to make it smaller.
- `TERM_LINE_GAP` 0 gives 34 rows instead of 30, but capitals and digits then
  touch the line above.

## Documentation

- [API reference](docs/api.md)
- [CONTRIBUTING.md](CONTRIBUTING.md): development setup, building, unit and
  hardware tests, reporting issues
- [ROADMAP.md](ROADMAP.md): planned features and known issues
- [DESIGN.md](DESIGN.md): design for the next phase of work
- [src/FONT.md](src/FONT.md): character codes and glyphs
- [AGENTS.md](AGENTS.md): design notes for AI coding agents

## License

titrmlib is licensed under the Apache License 2.0 ([LICENSE.md](https://github.com/gavinhsmith/titrmlib/blob/main/LICENSE.md)).
The font is derived from [petabyt/font](https://github.com/petabyt/font) (MIT,
[tools/petabyt-font/LICENSE](https://github.com/gavinhsmith/titrmlib/blob/main/tools/petabyt-font/LICENSE)).
