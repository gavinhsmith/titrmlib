

# Limits

## Macros

| Name | Description |
|------|-------------|
| [`TERM_MAX_PANELS`](#term_max_panels)  | Panels available, including the root and every scene and overlay. |
| [`TERM_INPUT_MAX`](#term_input_max)  | Longest text an input widget can hold. |

---

### TERM_MAX_PANELS

```cpp
#define TERM_MAX_PANELS 32
```

Defined in src/titrm.h:68

Panels available, including the root and every scene and overlay.

Each costs about 80 bytes of RAM whether used or not. Programs with few panels can lower it by defining it when building titrmlib, e.g. `CFLAGS += -DTERM_MAX_PANELS=12`.

---

### TERM_INPUT_MAX

```cpp
#define TERM_INPUT_MAX 48
```

Defined in src/titrm.h:72

Longest text an input widget can hold.

