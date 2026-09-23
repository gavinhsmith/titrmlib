

# Grid

The character grid and cell attributes.

## Macros

| Name | Description |
|------|-------------|
| [`TERM_ATTR_NORMAL`](#term_attr_normal)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): normal video. |
| [`TERM_ATTR_REVERSE`](#term_attr_reverse)  | Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): inverse video, the only "style" for now. |

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

Cell attribute for [term_panel_set_attr()](api-output.md#term_panel_set_attr): inverse video, the only "style" for now.

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

