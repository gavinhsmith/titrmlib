

# Panel tree

Splitting the screen into panels and sizing them.

## Classes

| Name | Description |
|------|-------------|
| [`term_size_t`](#term_size_t) | A panel's size within its parent. Build one with TERM_FIXED, TERM_PERCENT, TERM_FILL or TERM_FILL_WEIGHT. |

## Macros

| Name | Description |
|------|-------------|
| [`TERM_FIXED`](#term_fixed)  | Exactly `cells` cells. |
| [`TERM_PERCENT`](#term_percent)  | `pct` percent of the parent. |
| [`TERM_FILL`](#term_fill)  | An equal share of the space left after fixed and percent siblings. |
| [`TERM_FILL_WEIGHT`](#term_fill_weight)  | A share of the space left, weighted by `w` against other fill siblings. |

---

### TERM_FIXED

```cpp
#define TERM_FIXED(cells) ((term_size_t){TERM_SIZE_FIXED, (cells)})
```

Defined in src/titrm.h:318

Exactly `cells` cells.

---

### TERM_PERCENT

```cpp
#define TERM_PERCENT(pct) ((term_size_t){TERM_SIZE_PERCENT, (pct)})
```

Defined in src/titrm.h:320

`pct` percent of the parent.

---

### TERM_FILL

```cpp
#define TERM_FILL ((term_size_t){TERM_SIZE_FILL, 1})
```

Defined in src/titrm.h:322

An equal share of the space left after fixed and percent siblings.

---

### TERM_FILL_WEIGHT

```cpp
#define TERM_FILL_WEIGHT(w) ((term_size_t){TERM_SIZE_FILL, (w)})
```

Defined in src/titrm.h:324

A share of the space left, weighted by `w` against other fill siblings.

## Enumerations

| Name | Description |
|------|-------------|
| [`term_dir_t`](#term_dir_t)  | How a panel lays out its children. |
| [`term_size_kind`](#term_size_kind)  | Values of [term_size_t.kind](#kind). |

---

### term_dir_t

```cpp
enum term_dir_t
```

Defined in src/titrm.h:303

How a panel lays out its children.

| Value | Description |
|-------|-------------|
| `TERM_HORIZONTAL` | children sit side by side, left to right |
| `TERM_VERTICAL` | children are stacked, top to bottom |

---

### term_size_kind

```cpp
enum term_size_kind
```

Defined in src/titrm.h:315

Values of [term_size_t.kind](#kind).

| Value | Description |
|-------|-------------|
| `TERM_SIZE_FIXED` |  |
| `TERM_SIZE_PERCENT` |  |
| `TERM_SIZE_FILL` |  |
## Functions

| Return | Name | Description |
|--------|------|-------------|
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_root`](#term_root)  | The first scene's root panel, created by [term_init()](api-lifecycle.md#term_init). It covers the whole grid. |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_split`](#term_split)  | Adds a child to `parent` and returns it. |
| `void` | [`term_panel_destroy`](#term_panel_destroy)  | Removes a panel and everything below it. Panel handles become invalid. |
| `void` | [`term_panel_show`](#term_panel_show)  | Shows or hides a panel. Hidden panels take no space; siblings reflow. Takes effect next frame. |
| `bool` | [`term_panel_visible`](#term_panel_visible)  | Whether the panel is shown (see [term_panel_show()](#term_panel_show)). |
| `void` | [`term_panel_set_border`](#term_panel_set_border)  | Draws a box around the panel; its content area shrinks by one cell. |
| `void` | [`term_panel_set_title`](#term_panel_set_title)  | Sets a title into the top edge of the border. May be NULL; must outlive the panel. |
| `int` | [`term_panel_width`](#term_panel_width)  | Content width in cells (inside the border). |
| `int` | [`term_panel_height`](#term_panel_height)  | Content height in cells (inside the border). |

---

### term_root

```cpp
term_panel_t * term_root(term_ctx_t * ctx)
```

Defined in src/titrm.h:327

The first scene's root panel, created by [term_init()](api-lifecycle.md#term_init). It covers the whole grid.

---

### term_split

```cpp
term_panel_t * term_split(term_panel_t * parent, term_dir_t dir, term_size_t size)
```

Defined in src/titrm.h:339

Adds a child to `parent` and returns it.

The first split makes `parent` a container that lays its children out along `dir`; later splits append more children and must use the same `dir` (else NULL is returned, as it is when the panel pool is exhausted). A container's own content is not drawn, apart from its border. Fixed and percent sizes are taken first, then fill panels divide what remains by weight; children that don't fit are clipped.

---

### term_panel_destroy

```cpp
void term_panel_destroy(term_panel_t * panel)
```

Defined in src/titrm.h:347

Removes a panel and everything below it. Panel handles become invalid.

A scene that isn't active can be removed this way, with its overlays; the active scene and [term_root()](#term_root) can't. Removing an overlay closes it.

---

### term_panel_show

```cpp
void term_panel_show(term_panel_t * panel, bool visible)
```

Defined in src/titrm.h:350

Shows or hides a panel. Hidden panels take no space; siblings reflow. Takes effect next frame.

---

### term_panel_visible

```cpp
bool term_panel_visible(const term_panel_t * panel)
```

Defined in src/titrm.h:353

Whether the panel is shown (see [term_panel_show()](#term_panel_show)).

---

### term_panel_set_border

```cpp
void term_panel_set_border(term_panel_t * panel, bool border)
```

Defined in src/titrm.h:356

Draws a box around the panel; its content area shrinks by one cell.

---

### term_panel_set_title

```cpp
void term_panel_set_title(term_panel_t * panel, const char * title)
```

Defined in src/titrm.h:359

Sets a title into the top edge of the border. May be NULL; must outlive the panel.

---

### term_panel_width

```cpp
int term_panel_width(const term_panel_t * panel)
```

Defined in src/titrm.h:362

Content width in cells (inside the border).

---

### term_panel_height

```cpp
int term_panel_height(const term_panel_t * panel)
```

Defined in src/titrm.h:365

Content height in cells (inside the border).


## Class Definitions



### term_size_t

```cpp
#include <titrm.h>
```

```cpp
struct term_size_t
```

Defined in src/titrm.h:309

A panel's size within its parent. Build one with TERM_FIXED, TERM_PERCENT, TERM_FILL or TERM_FILL_WEIGHT.

#### Public Attributes

| Return | Name | Description |
|--------|------|-------------|
| `uint8_t` | [`kind`](#kind)  | TERM_SIZE_FIXED, TERM_SIZE_PERCENT or TERM_SIZE_FILL |
| `uint8_t` | [`value`](#value-1)  | cells, percent or fill weight |

---

##### kind

```cpp
uint8_t kind
```

Defined in src/titrm.h:310

TERM_SIZE_FIXED, TERM_SIZE_PERCENT or TERM_SIZE_FILL

---

##### value

```cpp
uint8_t value
```

Defined in src/titrm.h:311

cells, percent or fill weight

