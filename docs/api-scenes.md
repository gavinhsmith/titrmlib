

# Scenes

Full-screen panel trees the app switches between.

A scene is a root panel covering the whole grid, holding the panels of one screen. Only the active scene is shown and gets events. Scenes that aren't active keep their content, so switching back is cheap. All scenes share the pool of TERM_MAX_PANELS panels.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_scene_new`](#term_scene_new)  | Creates a scene and returns its root panel, or NULL if the pool is full. `handler` may be NULL. |
| `void` | [`term_scene_switch`](#term_scene_switch)  | Makes `scene` the one shown and receiving events. |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_scene_active`](#term_scene_active)  | The active scene's root panel. |

---

### term_scene_new

```cpp
term_panel_t * term_scene_new(term_ctx_t * ctx, term_update_fn handler, void * state)
```

Defined in src/titrm.h:199

Creates a scene and returns its root panel, or NULL if the pool is full. `handler` may be NULL.

---

### term_scene_switch

```cpp
void term_scene_switch(term_ctx_t * ctx, term_panel_t * scene)
```

Defined in src/titrm.h:208

Makes `scene` the one shown and receiving events.

Sends TERM_EV_SCENE_LEAVE to the old scene's handler and TERM_EV_SCENE_ENTER to the new one's. Focus doesn't move: if it was on a panel of the old scene, it becomes empty and TERM_EV_FOCUS_LOST follows.

---

### term_scene_active

```cpp
term_panel_t * term_scene_active(const term_ctx_t * ctx)
```

Defined in src/titrm.h:211

The active scene's root panel.

