

# Panel-scoped output

Drawing text into a panel.

Everything here is clipped to the panel's content area, whatever its depth in the tree. Panels are redrawn from scratch every frame: put output in a draw callback rather than expecting it to persist.

## Typedefs

| Return | Name | Description |
|--------|------|-------------|
| `void(*)` | [`term_draw_fn`](#term_draw_fn)  | A draw callback; see [term_panel_set_draw()](#term_panel_set_draw). |

---

### term_draw_fn

```cpp
using term_draw_fn = void(*)
```

Defined in src/titrm.h:263

A draw callback; see [term_panel_set_draw()](#term_panel_set_draw).

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_panel_set_draw`](#term_panel_set_draw)  | Sets the panel's draw callback, called each frame with a blank panel, its cursor at 0,0. |
| `void` | [`term_panel_move`](#term_panel_move)  | Moves the panel's cursor. |
| `void` | [`term_panel_set_attr`](#term_panel_set_attr)  | Sets the attribute (TERM_ATTR_*) for the text printed next. |
| `void` | [`term_panel_wrap`](#term_panel_wrap)  | Wrap at the right edge instead of clipping (the default). |
| `void` | [`term_panel_putc`](#term_panel_putc)  | Prints one character. '\n' starts a new line. |
| `void` | [`term_panel_print`](#term_panel_print)  | Prints a string at the cursor. |
| `void` | [`term_panel_printf`](#term_panel_printf)  | Prints formatted text at the cursor. |
| `void` | [`term_panel_repeat`](#term_panel_repeat)  | Prints `c``count` times. |
| `void` | [`term_panel_clear`](#term_panel_clear)  | Blanks the panel's content area and moves the cursor to 0,0. |

---

### term_panel_set_draw

```cpp
void term_panel_set_draw(term_panel_t * panel, term_draw_fn draw, void * user)
```

Defined in src/titrm.h:266

Sets the panel's draw callback, called each frame with a blank panel, its cursor at 0,0.

---

### term_panel_move

```cpp
void term_panel_move(term_panel_t * panel, int col, int row)
```

Defined in src/titrm.h:269

Moves the panel's cursor.

---

### term_panel_set_attr

```cpp
void term_panel_set_attr(term_panel_t * panel, uint8_t attr)
```

Defined in src/titrm.h:272

Sets the attribute (TERM_ATTR_*) for the text printed next.

---

### term_panel_wrap

```cpp
void term_panel_wrap(term_panel_t * panel, bool wrap)
```

Defined in src/titrm.h:275

Wrap at the right edge instead of clipping (the default).

---

### term_panel_putc

```cpp
void term_panel_putc(term_panel_t * panel, char c)
```

Defined in src/titrm.h:278

Prints one character. '\n' starts a new line.

---

### term_panel_print

```cpp
void term_panel_print(term_panel_t * panel, const char * str)
```

Defined in src/titrm.h:281

Prints a string at the cursor.

---

### term_panel_printf

```cpp
void term_panel_printf(term_panel_t * panel, const char * fmt, ...)
```

Defined in src/titrm.h:284

Prints formatted text at the cursor.

---

### term_panel_repeat

```cpp
void term_panel_repeat(term_panel_t * panel, char c, int count)
```

Defined in src/titrm.h:287

Prints `c``count` times.

---

### term_panel_clear

```cpp
void term_panel_clear(term_panel_t * panel)
```

Defined in src/titrm.h:290

Blanks the panel's content area and moves the cursor to 0,0.

