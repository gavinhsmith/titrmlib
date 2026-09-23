# Phase 2 design

Status: agreed, not started. titrmlib is at v0.1.0, so the API may change
freely until v1.0.

Phase 2 reworks how titrmlib renders and routes input, then adds scenes,
overlays, a revised widget set and color. Function and type names below are
proposals and may change during implementation.

## Goals

- A typical update (one row or one widget changes) takes **under 50 ms**,
  measured by the `perf` hardware test. No eZ80 assembly.
- Key presses are never lost to a slow frame.
- Full-screen scenes the app can switch between, and overlay panels on top of
  them.
- A small set of built-in widgets, plus the pieces needed to build custom ones.
- Color.

## 1. Retained panels and change-based rendering

- Each panel keeps its own cells. Output (`term_panel_print`, `_printf`,
  `_putc`, …) is written into them and stays until it is changed or cleared.
- Any change to a panel marks it dirty. Each frame composes only the dirty
  panels into the screen grid, and only changed cells are drawn, as today.
- Draw callbacks (`term_panel_set_draw`) are removed.
- Widgets keep their state and redraw their own cells when it changes.
- The cell format includes foreground and background colors from the start,
  so section 6 doesn't change storage again.

## 2. Key queue

- Keys are read into a small queue. The keypad is also polled while a frame is
  being drawn, so presses during a slow frame are kept.
- The run loop handles queued keys in order, one event each.

## 3. Scenes

- A scene is a full-screen root panel holding the panels of one screen. An
  app can create several.
- One scene is active and shown at a time. An API call switches scenes, e.g.
  `term_scene_switch(ctx, scene)`.
- Hidden scenes keep their retained content, so switching back is cheap.
- A scene can have its own event handler. Only the active scene's handler
  receives events. The program also keeps one global handler.
- Switching sends `TERM_EV_SCENE_LEAVE` to the outgoing scene and
  `TERM_EV_SCENE_ENTER` to the incoming one.
- Switching scenes doesn't move focus; the app sets it.

## 4. Overlays

- Panels drawn on top of the active scene, e.g. dialogs.
- Positioned by an explicit rectangle, or centered at a given size.
- Not modal: focus can move between an overlay and the panels under it.
- Opening an overlay doesn't move focus. It records the panel that had focus.
- Closing an overlay restores that panel's focus, but only if focus is inside
  the overlay or empty and the recorded panel still exists and is visible.
  Otherwise focus is left as it is. Stacked overlays each restore their own.
- Closing an overlay re-composes the retained panels underneath.

## 5. Focus

- The app decides where focus goes. titrmlib keeps the mechanism:
  `term_focus(ctx, panel)` and `term_focused(ctx)`.
- `term_focus_next` is removed.
- `[vars]` is an ordinary key, renamed `TERM_KEY_VARS` (was `TERM_KEY_TAB`).
- Nothing has focus at startup until the app sets it.
- If the focused panel is hidden or destroyed, focus becomes empty and the app
  receives `TERM_EV_FOCUS_LOST`.
- Restoring focus after an overlay (section 4) is the only time titrmlib moves
  focus by itself.

## 6. Events

Events pass along a chain until one step consumes them:

1. the focused widget
2. the active scene's handler
3. the global handler

Ticks go to the active scene's handler and the global handler.

| Event | Meaning |
|---|---|
| `TERM_EV_START` | Once, before the first frame |
| `TERM_EV_KEY` | A key nothing earlier in the chain consumed |
| `TERM_EV_TICK` | The interval set with `term_set_tick()` passed |
| `TERM_EV_SUBMIT` | The user confirmed with `[enter]`: a button, an input, or a list item (`value` = item index) |
| `TERM_EV_CHANGE` | A widget's value changed: input text edited, list selection moved, checkbox toggled |
| `TERM_EV_FOCUS_LOST` | The focused panel was hidden or destroyed |
| `TERM_EV_SCENE_ENTER` / `TERM_EV_SCENE_LEAVE` | The scene became active or inactive |

`TERM_EV_SELECT` is removed; choosing a list item sends `TERM_EV_SUBMIT`.
Custom widgets send the same events.

## 7. Widgets

Widgets are panels with general properties, not one-off code:

- text alignment (left or centered)
- attributes and colors
- focusable
- `[enter]` sends `TERM_EV_SUBMIT`
- an optional key handler, for custom widgets

Built-in widgets:

| Widget | Behavior |
|---|---|
| Container | A plain panel that holds children. Border and title are optional fields |
| Text | Static or word-wrapped text, with a scroll offset, appending lines and optional auto-scroll to the bottom (replaces the log widget) |
| Input | Single-line editable text |
| List | Selectable items. Up/down move and send `TERM_EV_CHANGE`; `[enter]` sends `TERM_EV_SUBMIT` |
| Button | A focusable text panel, centered and reversed, that sends `TERM_EV_SUBMIT` on `[enter]`. `term_make_button` is a shortcut for those settings |
| Checkbox | A focusable on/off item with a label; `[enter]` toggles it and sends `TERM_EV_CHANGE` |
| Progress | A horizontal bar, value out of a maximum |

- Style is up to the developer. Widgets start with plain defaults and every
  visual choice (focused look, selected item, input cursor) can be set per
  widget. titrmlib has no theme system.
- Anything else (status bars, tabs, dialogs, softkey bars, tables, spinners)
  is built by the developer from these widgets. The demo shows how.

## 8. Color

- Per-cell foreground and background colors, set per panel the same way
  attributes are today.
- The two-color `row_pixels` table in `draw_cell` is replaced by drawing that
  handles any color pair.
- Colors are graphx palette indices.

## Build order

1. Retained panels, change-based rendering and the key queue (sections 1–2)
2. Focus and event changes (sections 5–6)
3. Scenes (section 3)
4. Overlays (section 4)
5. Widget set (section 7), and the demo rebuilt on it
6. Color (section 8)

Each step keeps the unit tests and hardware tests passing, with screens
re-recorded where output changes on purpose. The `perf` budgets tighten to
the 50 ms goal once step 1 lands.

## Still to decide

- The panel pool: whether `TERM_MAX_PANELS` (32) is enough with several
  scenes and overlays, or whether it should grow or be per scene.
- Memory: retained cells cost roughly one screen of cells per scene plus
  overlays. Budget and limits to be checked on hardware.
- Exact text widget API for appending and scrolling.
- Input options: maximum length, digits only, password masking.
- List options: multi-select with check marks.
