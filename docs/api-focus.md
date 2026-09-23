

# Focus

Which panel receives keys.

Widgets that take input (list, input, log) are focusable by default; any panel can opt in. [vars] moves focus forward in tree order.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `void` | [`term_panel_set_focusable`](#term_panel_set_focusable)  | Lets a panel take focus, or stops it. |
| `void` | [`term_focus`](#term_focus)  | Moves focus to `panel` (making it focusable), or clears it with NULL. |
| `void` | [`term_focus_next`](#term_focus_next)  | Moves focus to the next focusable panel in tree order, as [vars] does. |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`term_focused`](#term_focused)  | The focused panel, or NULL. |

---

### term_panel_set_focusable

```cpp
void term_panel_set_focusable(term_panel_t * panel, bool focusable)
```

Defined in src/titrm.h:249

Lets a panel take focus, or stops it.

---

### term_focus

```cpp
void term_focus(term_ctx_t * ctx, term_panel_t * panel)
```

Defined in src/titrm.h:252

Moves focus to `panel` (making it focusable), or clears it with NULL.

---

### term_focus_next

```cpp
void term_focus_next(term_ctx_t * ctx)
```

Defined in src/titrm.h:255

Moves focus to the next focusable panel in tree order, as [vars] does.

---

### term_focused

```cpp
term_panel_t * term_focused(const term_ctx_t * ctx)
```

Defined in src/titrm.h:258

The focused panel, or NULL.

