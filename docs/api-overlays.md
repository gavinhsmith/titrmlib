

# Overlays

Panels drawn on top of the active scene, such as dialogs.

An overlay is a root panel with its own position and size, drawn over the scene that was active when it opened, and shown only while that scene is. Overlays are opaque and drawn in the order they were opened, the newest on top. They aren't modal: focus can move between an overlay and the panels under it.

Opening an overlay doesn't move focus; it remembers the panel that had it. Closing the overlay gives focus back to that panel, but only if focus is in the overlay or empty, and the panel still exists and is on screen.

## Macros

| Name | Description |
|------|-------------|
| [`TERM_MAX_OVERLAYS`](#term_max_overlays)  | Up to this many overlays can be open at once. |

---

### TERM_MAX_OVERLAYS

```cpp
#define TERM_MAX_OVERLAYS 8
```

Defined in src/titrm.h:282

Up to this many overlays can be open at once.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_overlay_open`](#term_overlay_open)  | Opens an overlay at column `col`, row `row`, `w` by `h` cells (clipped to the grid). NULL if the pool or overlay limit is full. |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_overlay_open_centered`](#term_overlay_open_centered)  | Opens an overlay of `w` by `h` cells, centered on the grid. |
| `void` | [`term_overlay_close`](#term_overlay_close)  | Closes an overlay, removing it and its panels, and gives focus back as described above. |

---

### term_overlay_open

```cpp
term_panel_t * term_overlay_open(term_ctx_t * ctx, int col, int row, int w, int h)
```

Defined in src/titrm.h:285

Opens an overlay at column `col`, row `row`, `w` by `h` cells (clipped to the grid). NULL if the pool or overlay limit is full.

---

### term_overlay_open_centered

```cpp
term_panel_t * term_overlay_open_centered(term_ctx_t * ctx, int w, int h)
```

Defined in src/titrm.h:288

Opens an overlay of `w` by `h` cells, centered on the grid.

---

### term_overlay_close

```cpp
void term_overlay_close(term_panel_t * overlay)
```

Defined in src/titrm.h:291

Closes an overlay, removing it and its panels, and gives focus back as described above.

