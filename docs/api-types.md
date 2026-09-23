

# Handles

Opaque handles passed to every call.

## Typedefs

| Return | Name | Description |
|--------|------|-------------|
| `struct term_ctx` | [`term_ctx_t`](#term_ctx_t)  | The framework context. There is only one; see [term_init()](api-lifecycle.md#term_init). |
| `struct term_panel` | [`term_panel_t`](#term_panel_t)  | A panel in the tree. Handles stay valid until the panel is destroyed. |

---

### term_ctx_t

```cpp
using term_ctx_t = struct term_ctx
```

Defined in src/titrm.h:47

The framework context. There is only one; see [term_init()](api-lifecycle.md#term_init).

---

### term_panel_t

```cpp
using term_panel_t = struct term_panel
```

Defined in src/titrm.h:50

A panel in the tree. Handles stay valid until the panel is destroyed.

