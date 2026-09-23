

# Widgets

Panels with built-in content and key handling.

term_make_* turns a panel into a widget; the term_<widget>_* calls drive it.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_make_text`](#term_make_text)  | Static text, word-wrapped to the panel. `text` must outlive the panel. |
| `void` | [`term_text_set`](#term_text_set)  | Replaces a text widget's text. `text` must outlive the panel. |
| `void` | [`term_make_list`](#term_make_list)  | Selectable list. up/down move, [enter] emits TERM_EV_SELECT. |
| `void` | [`term_list_set_items`](#term_list_set_items)  | Replaces a list's items. Items are not copied. |
| `int` | [`term_list_selected`](#term_list_selected)  | Index of the selected item, or -1 if the list is empty. |
| `void` | [`term_list_select`](#term_list_select)  | Selects an item; out-of-range indexes are clamped. |
| `void` | [`term_make_input`](#term_make_input)  | Single-line text field. |
| `const char *` | [`term_input_text`](#term_input_text)  | The input's current text. |
| `void` | [`term_input_set`](#term_input_set)  | Replaces the input's text (up to TERM_INPUT_MAX characters). |
| `void` | [`term_make_log`](#term_make_log)  | Scrollback holding up to `max_lines`. New lines are appended at the bottom; up/down scroll back. |
| `void` | [`term_log_print`](#term_log_print)  | Appends text to a log; each '\n' starts a new line. |
| `void` | [`term_log_printf`](#term_log_printf)  | Appends formatted text to a log. |
| `void` | [`term_log_clear`](#term_log_clear)  | Removes every line from a log. |
| `void` | [`term_make_progress`](#term_make_progress)  | Horizontal progress bar filling the panel's first row, from 0 to `max`. |
| `void` | [`term_progress_set`](#term_progress_set)  | Sets the progress value; it is clamped to 0..max. |

---

### term_make_text

```cpp
void term_make_text(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:303

Static text, word-wrapped to the panel. `text` must outlive the panel.

---

### term_text_set

```cpp
void term_text_set(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:306

Replaces a text widget's text. `text` must outlive the panel.

---

### term_make_list

```cpp
void term_make_list(term_panel_t * panel, const char *const * items, int count)
```

Defined in src/titrm.h:313

Selectable list. up/down move, [enter] emits TERM_EV_SELECT.

Items are not copied. Embed icons with TERM_S_* (e.g. TERM_S_CHECK "Done").

---

### term_list_set_items

```cpp
void term_list_set_items(term_panel_t * panel, const char *const * items, int count)
```

Defined in src/titrm.h:316

Replaces a list's items. Items are not copied.

---

### term_list_selected

```cpp
int term_list_selected(const term_panel_t * panel)
```

Defined in src/titrm.h:319

Index of the selected item, or -1 if the list is empty.

---

### term_list_select

```cpp
void term_list_select(term_panel_t * panel, int index)
```

Defined in src/titrm.h:322

Selects an item; out-of-range indexes are clamped.

---

### term_make_input

```cpp
void term_make_input(term_panel_t * panel)
```

Defined in src/titrm.h:330

Single-line text field.

Typing inserts, [del] backspaces, [clear] empties, left/right move the cursor, [enter] emits TERM_EV_SUBMIT.

---

### term_input_text

```cpp
const char * term_input_text(const term_panel_t * panel)
```

Defined in src/titrm.h:333

The input's current text.

---

### term_input_set

```cpp
void term_input_set(term_panel_t * panel, const char * text)
```

Defined in src/titrm.h:336

Replaces the input's text (up to TERM_INPUT_MAX characters).

---

### term_make_log

```cpp
void term_make_log(term_panel_t * panel, int max_lines)
```

Defined in src/titrm.h:339

Scrollback holding up to `max_lines`. New lines are appended at the bottom; up/down scroll back.

---

### term_log_print

```cpp
void term_log_print(term_panel_t * panel, const char * str)
```

Defined in src/titrm.h:342

Appends text to a log; each '\n' starts a new line.

---

### term_log_printf

```cpp
void term_log_printf(term_panel_t * panel, const char * fmt, ...)
```

Defined in src/titrm.h:345

Appends formatted text to a log.

---

### term_log_clear

```cpp
void term_log_clear(term_panel_t * panel)
```

Defined in src/titrm.h:348

Removes every line from a log.

---

### term_make_progress

```cpp
void term_make_progress(term_panel_t * panel, int max)
```

Defined in src/titrm.h:351

Horizontal progress bar filling the panel's first row, from 0 to `max`.

---

### term_progress_set

```cpp
void term_progress_set(term_panel_t * panel, int value)
```

Defined in src/titrm.h:354

Sets the progress value; it is clamped to 0..max.

