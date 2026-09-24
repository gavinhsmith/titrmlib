

# Grid

The character grid and cell attributes.

## Macros

| Name | Description |
|------|-------------|
| [`TERM_ATTR_NORMAL`](#term_attr_normal)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): normal video. |
| [`TERM_ATTR_REVERSE`](#term_attr_reverse)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): the panel's colors swapped. |
| [`TERM_ATTR_BOLD`](#term_attr_bold)  | Cell attribute: bold, each glyph thickened one pixel to the right. |
| [`TERM_ATTR_ITALIC`](#term_attr_italic)  | Cell attribute: italic, the top of each glyph slanted one pixel right. |
| [`TERM_ATTR_UNDERLINE`](#term_attr_underline)  | Cell attribute: underlined, joining across cells. |
| [`TERM_ATTR_STRIKE`](#term_attr_strike)  | Cell attribute: struck through, joining across cells. |
| [`TERM_S_NORMAL`](#term_s_normal)  | Inline style: back to normal, from here on in a string. |
| [`TERM_S_REVERSE`](#term_s_reverse)  | Inline style: reverse video (see TERM_S_NORMAL). |
| [`TERM_S_BOLD`](#term_s_bold)  | Inline style: bold (see TERM_S_NORMAL). |
| [`TERM_S_ITALIC`](#term_s_italic)  | Inline style: italic (see TERM_S_NORMAL). |
| [`TERM_S_UNDERLINE`](#term_s_underline)  | Inline style: underlined (see TERM_S_NORMAL). |
| [`TERM_S_STRIKE`](#term_s_strike)  | Inline style: struck through (see TERM_S_NORMAL). |
| [`TERM_COLOR_BLACK`](#term_color_black)  | the default background |
| [`TERM_COLOR_WHITE`](#term_color_white)  | the default foreground |
| [`TERM_COLOR_RED`](#term_color_red)  | red |
| [`TERM_COLOR_ORANGE`](#term_color_orange)  | orange (graphx names 0xE3, which shows as yellow) |
| [`TERM_COLOR_YELLOW`](#term_color_yellow)  | yellow |
| [`TERM_COLOR_GREEN`](#term_color_green)  | green |
| [`TERM_COLOR_BLUE`](#term_color_blue)  | blue |
| [`TERM_COLOR_PURPLE`](#term_color_purple)  | purple |
| [`TERM_COLOR_PINK`](#term_color_pink)  | pink |

---

### TERM_ATTR_NORMAL

```cpp
#define TERM_ATTR_NORMAL 0
```

Defined in src/titrm.h:89

Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): normal video.

---

### TERM_ATTR_REVERSE

```cpp
#define TERM_ATTR_REVERSE 1
```

Defined in src/titrm.h:92

Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): the panel's colors swapped.

---

### TERM_ATTR_BOLD

```cpp
#define TERM_ATTR_BOLD 2
```

Defined in src/titrm.h:95

Cell attribute: bold, each glyph thickened one pixel to the right.

---

### TERM_ATTR_ITALIC

```cpp
#define TERM_ATTR_ITALIC 4
```

Defined in src/titrm.h:98

Cell attribute: italic, the top of each glyph slanted one pixel right.

---

### TERM_ATTR_UNDERLINE

```cpp
#define TERM_ATTR_UNDERLINE 8
```

Defined in src/titrm.h:101

Cell attribute: underlined, joining across cells.

---

### TERM_ATTR_STRIKE

```cpp
#define TERM_ATTR_STRIKE 16
```

Defined in src/titrm.h:104

Cell attribute: struck through, joining across cells.

---

### TERM_S_NORMAL

```cpp
#define TERM_S_NORMAL "\x1b" "@"
```

Defined in src/titrm.h:117

Inline style: back to normal, from here on in a string.

The TERM_S_* inline styles are ESC (0x1B) followed by 0x40 | TERM_ATTR_* bits, so other combinations can be written the same way, e.g. `"\x1b" "J"` for bold and underlined. They take no space. Printed with [term_panel_print()](api-output.md#term_panel_print) and friends, they set the panel's attribute; in a text widget, list item or label they add to the widget's attribute until the next one or the end of the line. An ESC not followed by 0x40-0x5F is dropped.

---

### TERM_S_REVERSE

```cpp
#define TERM_S_REVERSE "\x1b" "A"
```

Defined in src/titrm.h:118

Inline style: reverse video (see TERM_S_NORMAL).

---

### TERM_S_BOLD

```cpp
#define TERM_S_BOLD "\x1b" "B"
```

Defined in src/titrm.h:119

Inline style: bold (see TERM_S_NORMAL).

---

### TERM_S_ITALIC

```cpp
#define TERM_S_ITALIC "\x1b" "D"
```

Defined in src/titrm.h:120

Inline style: italic (see TERM_S_NORMAL).

---

### TERM_S_UNDERLINE

```cpp
#define TERM_S_UNDERLINE "\x1b" "H"
```

Defined in src/titrm.h:121

Inline style: underlined (see TERM_S_NORMAL).

---

### TERM_S_STRIKE

```cpp
#define TERM_S_STRIKE "\x1b" "P"
```

Defined in src/titrm.h:122

Inline style: struck through (see TERM_S_NORMAL).

---

### TERM_COLOR_BLACK

```cpp
#define TERM_COLOR_BLACK 0x00
```

Defined in src/titrm.h:126

the default background

---

### TERM_COLOR_WHITE

```cpp
#define TERM_COLOR_WHITE 0xFF
```

Defined in src/titrm.h:127

the default foreground

---

### TERM_COLOR_RED

```cpp
#define TERM_COLOR_RED 0xE0
```

Defined in src/titrm.h:128

red

---

### TERM_COLOR_ORANGE

```cpp
#define TERM_COLOR_ORANGE 0xE1
```

Defined in src/titrm.h:129

orange (graphx names 0xE3, which shows as yellow)

---

### TERM_COLOR_YELLOW

```cpp
#define TERM_COLOR_YELLOW 0xE7
```

Defined in src/titrm.h:130

yellow

---

### TERM_COLOR_GREEN

```cpp
#define TERM_COLOR_GREEN 0x03
```

Defined in src/titrm.h:131

green

---

### TERM_COLOR_BLUE

```cpp
#define TERM_COLOR_BLUE 0x10
```

Defined in src/titrm.h:132

blue

---

### TERM_COLOR_PURPLE

```cpp
#define TERM_COLOR_PURPLE 0x50
```

Defined in src/titrm.h:133

purple

---

### TERM_COLOR_PINK

```cpp
#define TERM_COLOR_PINK 0xF0
```

Defined in src/titrm.h:134

pink

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `int` | [`term_cols`](#term_cols)  | Grid width in cells: 53 with the built-in 5x7 font (6x8 cells). |
| `int` | [`term_rows`](#term_rows)  | Grid height in cells: 30 with the built-in 5x7 font (6x8 cells). |

---

### term_cols

```cpp
int term_cols(void)
```

Defined in src/titrm.h:83

Grid width in cells: 53 with the built-in 5x7 font (6x8 cells).

---

### term_rows

```cpp
int term_rows(void)
```

Defined in src/titrm.h:86

Grid height in cells: 30 with the built-in 5x7 font (6x8 cells).

