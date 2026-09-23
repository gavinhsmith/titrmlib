

# Focus

Which panel receives keys.

Keys go to the focused panel's widget first, and to the app if the widget doesn't use them. The app decides where focus goes: titrmlib never moves it on its own. Nothing is focused until the app calls [term_focus()](#term_focus). If the focused panel is hidden or destroyed, focus becomes empty and the app gets TERM_EV_FOCUS_LOST.

Widgets that take input (list, input, log) are focusable by default; any other panel can opt in with [term_panel_set_focusable()](#term_panel_set_focusable).

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_panel_set_focusable`](#term_panel_set_focusable)  | Lets a panel take focus, or stops it. |
| `void` | [`term_focus`](#term_focus)  | Moves focus to `panel`, or clears it with NULL. Ignored for a panel that isn't focusable. |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_focused`](#term_focused)  | The focused panel, or NULL. |

---

### term_panel_set_focusable

```cpp
void term_panel_set_focusable(term_panel_t * panel, bool focusable)
```

Defined in src/titrm.h:257

Lets a panel take focus, or stops it.

---

### term_focus

```cpp
void term_focus(term_ctx_t * ctx, term_panel_t * panel)
```

Defined in src/titrm.h:260

Moves focus to `panel`, or clears it with NULL. Ignored for a panel that isn't focusable.

---

### term_focused

```cpp
term_panel_t * term_focused(const term_ctx_t * ctx)
```

Defined in src/titrm.h:263

The focused panel, or NULL.

