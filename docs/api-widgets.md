

# Widgets

Panels with built-in content and key handling.

term_make_* turns a panel with no children into a widget; the term_<widget>_* calls drive it. A plain panel that holds other panels is a container, and needs no call: give it a border and title if you like.

Widgets draw in the panel's attribute ([term_panel_set_attr()](api-output.md#term_panel_set_attr)) and show focus with its focus attribute ([term_panel_set_focus_attr()](api-custom.md#term_panel_set_focus_attr)).

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_make_text`](#term_make_text)  | Text, word-wrapped to the panel. The text is copied. |
| `void` | [`term_text_set`](#term_text_set)  | Replaces a text widget's text (copied). |
| `void` | [`term_text_append`](#term_text_append)  | Adds text to the end. Use '\n' to end lines, e.g. for a log. |
| `void` | [`term_text_appendf`](#term_text_appendf)  | Adds formatted text to the end, formatted as in [term_panel_printf()](api-output.md#term_panel_printf). |
| `void` | [`term_text_clear`](#term_text_clear)  | Removes all the text. |
| `void` | [`term_text_limit`](#term_text_limit)  | Keeps at most `bytes` of text, dropping the oldest lines first. The default is 1024. |
| `void` | [`term_text_autoscroll`](#term_text_autoscroll)  | Keeps the end of the text in view as it grows, unless scrolled up; scrolling back to the end resumes it. |
| `void` | [`term_text_scroll`](#term_text_scroll)  | Scrolls by `rows` (negative is up), within the text. |
| `void` | [`term_make_button`](#term_make_button)  | A button: centered text in reverse video that sends TERM_EV_SUBMIT on [enter]. |
| `void` | [`term_make_checkbox`](#term_make_checkbox)  | An on/off item: "[x] label". [enter] toggles it and sends TERM_EV_CHANGE (value = 1 if checked). |
| `bool` | [`term_checkbox_checked`](#term_checkbox_checked)  | Whether the checkbox is checked. |
| `void` | [`term_checkbox_set`](#term_checkbox_set)  | Checks or unchecks the checkbox (sends no event). |
| `void` | [`term_make_list`](#term_make_list)  | Selectable list. up/down move (TERM_EV_CHANGE), [enter] emits TERM_EV_SUBMIT. |
| `void` | [`term_list_set_items`](#term_list_set_items)  | Replaces a list's items. Items are not copied. |
| `int` | [`term_list_selected`](#term_list_selected)  | Index of the selected item, or -1 if the list is empty. |
| `void` | [`term_list_select`](#term_list_select)  | Selects an item; out-of-range indexes are clamped. |
| `void` | [`term_make_input`](#term_make_input)  | Single-line text field. |
| `const char *` | [`term_input_text`](#term_input_text)  | The input's current text. |
| `void` | [`term_input_set`](#term_input_set)  | Replaces the input's text (up to TERM_INPUT_MAX characters). |
| `void` | [`term_make_progress`](#term_make_progress)  | Horizontal progress bar filling the panel's first row, from 0 to `max`. |
| `void` | [`term_progress_set`](#term_progress_set)  | Sets the progress value; it is clamped to 0..max. |

---

### term_make_text

```cpp
void term_make_text(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:435

Text, word-wrapped to the panel. The text is copied.

'\n' starts a new line. If the text is taller than the panel, a focusable text widget scrolls with up/down, and arrows at the right edge show that more is above or below.

---

### term_text_set

```cpp
void term_text_set(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:438

Replaces a text widget's text (copied).

---

### term_text_append

```cpp
void term_text_append(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:441

Adds text to the end. Use '\n' to end lines, e.g. for a log.

---

### term_text_appendf

```cpp
void term_text_appendf(term_panel_t * panel, const char * fmt, ...)
```

Defined in src/titrm.h:444

Adds formatted text to the end, formatted as in [term_panel_printf()](api-output.md#term_panel_printf).

---

### term_text_clear

```cpp
void term_text_clear(term_panel_t * panel)
```

Defined in src/titrm.h:447

Removes all the text.

---

### term_text_limit

```cpp
void term_text_limit(term_panel_t * panel, int bytes)
```

Defined in src/titrm.h:450

Keeps at most `bytes` of text, dropping the oldest lines first. The default is 1024.

---

### term_text_autoscroll

```cpp
void term_text_autoscroll(term_panel_t * panel, bool on)
```

Defined in src/titrm.h:453

Keeps the end of the text in view as it grows, unless scrolled up; scrolling back to the end resumes it.

---

### term_text_scroll

```cpp
void term_text_scroll(term_panel_t * panel, int rows)
```

Defined in src/titrm.h:456

Scrolls by `rows` (negative is up), within the text.

---

### term_make_button

```cpp
void term_make_button(term_panel_t * panel, const char * label)
```

Defined in src/titrm.h:464

A button: centered text in reverse video that sends TERM_EV_SUBMIT on [enter].

It's a focusable text widget with those settings, and an arrow at its left edge while it has focus. The label is copied.

---

### term_make_checkbox

```cpp
void term_make_checkbox(term_panel_t * panel, const char * label, bool checked)
```

Defined in src/titrm.h:467

An on/off item: "[x] label". [enter] toggles it and sends TERM_EV_CHANGE (value = 1 if checked).

---

### term_checkbox_checked

```cpp
bool term_checkbox_checked(const term_panel_t * panel)
```

Defined in src/titrm.h:470

Whether the checkbox is checked.

---

### term_checkbox_set

```cpp
void term_checkbox_set(term_panel_t * panel, bool checked)
```

Defined in src/titrm.h:473

Checks or unchecks the checkbox (sends no event).

---

### term_make_list

```cpp
void term_make_list(term_panel_t * panel, const char *const * items, int count)
```

Defined in src/titrm.h:480

Selectable list. up/down move (TERM_EV_CHANGE), [enter] emits TERM_EV_SUBMIT.

Items are not copied. Embed icons with TERM_S_* (e.g. TERM_S_SIG3 "HomeWiFi").

---

### term_list_set_items

```cpp
void term_list_set_items(term_panel_t * panel, const char *const * items, int count)
```

Defined in src/titrm.h:483

Replaces a list's items. Items are not copied.

---

### term_list_selected

```cpp
int term_list_selected(const term_panel_t * panel)
```

Defined in src/titrm.h:486

Index of the selected item, or -1 if the list is empty.

---

### term_list_select

```cpp
void term_list_select(term_panel_t * panel, int index)
```

Defined in src/titrm.h:489

Selects an item; out-of-range indexes are clamped.

---

### term_make_input

```cpp
void term_make_input(term_panel_t * panel)
```

Defined in src/titrm.h:497

Single-line text field.

Typing inserts, [del] backspaces, [clear] empties, left/right move the cursor, [enter] emits TERM_EV_SUBMIT.

---

### term_input_text

```cpp
const char * term_input_text(const term_panel_t * panel)
```

Defined in src/titrm.h:500

The input's current text.

---

### term_input_set

```cpp
void term_input_set(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:503

Replaces the input's text (up to TERM_INPUT_MAX characters).

---

### term_make_progress

```cpp
void term_make_progress(term_panel_t * panel, int max)
```

Defined in src/titrm.h:506

Horizontal progress bar filling the panel's first row, from 0 to `max`.

---

### term_progress_set

```cpp
void term_progress_set(term_panel_t * panel, int value)
```

Defined in src/titrm.h:509

Sets the progress value; it is clamped to 0..max.

