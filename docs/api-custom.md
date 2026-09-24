

# Panel properties and custom widgets

Settings any panel can have, and what's needed to build new widgets.

A custom widget is a panel with a key handler: make it focusable, print its content, handle keys while it's focused, and send TERM_EV_SUBMIT or TERM_EV_CHANGE with [term_panel_send()](#term_panel_send) so the app hears about it like any built-in widget.

## Enumerations

| Name | Description |
|------|-------------|
| [`term_align_t`](#term_align_t)  | Text alignment, for text widgets. |

---

### term_align_t

```cpp
enum term_align_t
```

Defined in src/titrm.h:565

Text alignment, for text widgets.

| Value | Description |
|-------|-------------|
| `TERM_ALIGN_LEFT` | the default |
| `TERM_ALIGN_CENTER` | each line centered |
## Typedefs

| Return | Name | Description |
|--------|------|-------------|
| `bool(*)` | [`term_key_fn`](#term_key_fn)  | A key handler for a custom widget; return true if the key was used. |

---

### term_key_fn

```cpp
using term_key_fn = bool(*)
```

Defined in src/titrm.h:585

A key handler for a custom widget; return true if the key was used.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_panel_set_align`](#term_panel_set_align)  | Aligns a text widget's lines. |
| `void` | [`term_panel_set_focus_attr`](#term_panel_set_focus_attr)  | Sets how the panel shows it has focus (TERM_ATTR_*; TERM_ATTR_REVERSE by default). |
| `void` | [`term_panel_set_submit`](#term_panel_set_submit)  | When on, [enter] on the focused panel sends TERM_EV_SUBMIT, if its widget doesn't use it. |
| `void` | [`term_panel_set_keys`](#term_panel_set_keys)  | Keys while `panel` is focused go to `fn` first, before its built-in widget. NULL removes it. |
| `void` | [`term_panel_send`](#term_panel_send)  | Sends an event from `panel` along the handler chain, as a widget does (e.g. TERM_EV_SUBMIT). |

---

### term_panel_set_align

```cpp
void term_panel_set_align(term_panel_t * panel, term_align_t align)
```

Defined in src/titrm.h:571

Aligns a text widget's lines.

---

### term_panel_set_focus_attr

```cpp
void term_panel_set_focus_attr(term_panel_t * panel, uint8_t attr)
```

Defined in src/titrm.h:579

Sets how the panel shows it has focus (TERM_ATTR_*; TERM_ATTR_REVERSE by default).

Used for the title, a list's selected row, an input's cursor and a focused checkbox. TERM_ATTR_NORMAL turns the highlight off.

---

### term_panel_set_submit

```cpp
void term_panel_set_submit(term_panel_t * panel, bool submit)
```

Defined in src/titrm.h:582

When on, [enter] on the focused panel sends TERM_EV_SUBMIT, if its widget doesn't use it.

---

### term_panel_set_keys

```cpp
void term_panel_set_keys(term_panel_t * panel, term_key_fn fn, void * state)
```

Defined in src/titrm.h:588

Keys while `panel` is focused go to `fn` first, before its built-in widget. NULL removes it.

---

### term_panel_send

```cpp
void term_panel_send(term_panel_t * panel, term_event_type_t type, int value)
```

Defined in src/titrm.h:591

Sends an event from `panel` along the handler chain, as a widget does (e.g. TERM_EV_SUBMIT).

