

# Panel-scoped output

Drawing text into a panel.

Output is retained: what is printed into a panel stays there, and is shown again every frame, until it is overwritten or cleared. The cursor and attribute persist too. Only the parts of the screen that change are redrawn.

Everything here is clipped to the panel's content area, whatever its depth in the tree. Only panels without children hold content; output to a panel that has been split is ignored. Resizing a panel keeps the part of its content that still fits, anchored at the top left.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_panel_move`](#term_panel_move)  | Moves the panel's cursor. |
| `void` | [`term_panel_set_attr`](#term_panel_set_attr)  | Sets the attribute (TERM_ATTR_*) for the text printed next. |
| `void` | [`term_panel_set_colors`](#term_panel_set_colors)  | Sets the panel's colors (TERM_COLOR_*, or any palette index). |
| `void` | [`term_panel_wrap`](#term_panel_wrap)  | Wrap at the right edge instead of clipping (the default). |
| `void` | [`term_panel_putc`](#term_panel_putc)  | Prints one character. '\n' starts a new line. |
| `void` | [`term_panel_print`](#term_panel_print)  | Prints a string at the cursor. |
| `void` | [`term_panel_printf`](#term_panel_printf)  | Prints formatted text at the cursor. |
| `void` | [`term_panel_repeat`](#term_panel_repeat)  | Prints `c``count` times. |
| `void` | [`term_panel_clear`](#term_panel_clear)  | Blanks the panel's content area and moves the cursor to 0,0. |

---

### term_panel_move

```cpp
void term_panel_move(term_panel_t * panel, int col, int row)
```

Defined in src/titrm.h:373

Moves the panel's cursor.

---

### term_panel_set_attr

```cpp
void term_panel_set_attr(term_panel_t * panel, uint8_t attr)
```

Defined in src/titrm.h:376

Sets the attribute (TERM_ATTR_*) for the text printed next.

---

### term_panel_set_colors

```cpp
void term_panel_set_colors(term_panel_t * panel, uint8_t fg, uint8_t bg)
```

Defined in src/titrm.h:386

Sets the panel's colors (TERM_COLOR_*, or any palette index).

Like the attribute, they apply to what is printed next, and clearing fills the panel with the background. Widgets, the border and the title are drawn in them, and a panel with children fills its area with its background. Panels split from this one start with its colors. White on black by default.

---

### term_panel_wrap

```cpp
void term_panel_wrap(term_panel_t * panel, bool wrap)
```

Defined in src/titrm.h:389

Wrap at the right edge instead of clipping (the default).

---

### term_panel_putc

```cpp
void term_panel_putc(term_panel_t * panel, char c)
```

Defined in src/titrm.h:392

Prints one character. '\n' starts a new line.

---

### term_panel_print

```cpp
void term_panel_print(term_panel_t * panel, const char * str)
```

Defined in src/titrm.h:395

Prints a string at the cursor.

---

### term_panel_printf

```cpp
void term_panel_printf(term_panel_t * panel, const char * fmt, ...)
```

Defined in src/titrm.h:405

Prints formatted text at the cursor.

A small printf of titrmlib's own, so programs don't link the toolchain's (about 7 KB). It supports `d u x X c s %%`, the `l` modifier and a width with the `-` and `0` flags, e.g. `%-10s`, `%05ld`, `%02X`. Other specifiers (floats, precision, `p`) are printed as written.

---

### term_panel_repeat

```cpp
void term_panel_repeat(term_panel_t * panel, char c, int count)
```

Defined in src/titrm.h:408

Prints `c``count` times.

---

### term_panel_clear

```cpp
void term_panel_clear(term_panel_t * panel)
```

Defined in src/titrm.h:411

Blanks the panel's content area and moves the cursor to 0,0.

