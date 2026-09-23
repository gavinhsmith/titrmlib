

# Grid

The character grid and cell attributes.

## Macros

| Name | Description |
|------|-------------|
| [`TERM_ATTR_NORMAL`](#term_attr_normal)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): normal video. |
| [`TERM_ATTR_REVERSE`](#term_attr_reverse)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): the panel's colors swapped. |
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

Defined in src/titrm.h:80

Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): normal video.

---

### TERM_ATTR_REVERSE

```cpp
#define TERM_ATTR_REVERSE 1
```

Defined in src/titrm.h:83

Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): the panel's colors swapped.

---

### TERM_COLOR_BLACK

```cpp
#define TERM_COLOR_BLACK 0x00
```

Defined in src/titrm.h:87

the default background

---

### TERM_COLOR_WHITE

```cpp
#define TERM_COLOR_WHITE 0xFF
```

Defined in src/titrm.h:88

the default foreground

---

### TERM_COLOR_RED

```cpp
#define TERM_COLOR_RED 0xE0
```

Defined in src/titrm.h:89

red

---

### TERM_COLOR_ORANGE

```cpp
#define TERM_COLOR_ORANGE 0xE1
```

Defined in src/titrm.h:90

orange (graphx names 0xE3, which shows as yellow)

---

### TERM_COLOR_YELLOW

```cpp
#define TERM_COLOR_YELLOW 0xE7
```

Defined in src/titrm.h:91

yellow

---

### TERM_COLOR_GREEN

```cpp
#define TERM_COLOR_GREEN 0x03
```

Defined in src/titrm.h:92

green

---

### TERM_COLOR_BLUE

```cpp
#define TERM_COLOR_BLUE 0x10
```

Defined in src/titrm.h:93

blue

---

### TERM_COLOR_PURPLE

```cpp
#define TERM_COLOR_PURPLE 0x50
```

Defined in src/titrm.h:94

purple

---

### TERM_COLOR_PINK

```cpp
#define TERM_COLOR_PINK 0xF0
```

Defined in src/titrm.h:95

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

Defined in src/titrm.h:74

Grid width in cells: 53 with the built-in 5x7 font (6x8 cells).

---

### term_rows

```cpp
int term_rows(void)
```

Defined in src/titrm.h:77

Grid height in cells: 30 with the built-in 5x7 font (6x8 cells).

