

# Lifecycle

Starting, running and stopping the framework.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| [`term_ctx_t`](api-types.md#term_ctx_t) * | [`term_init`](#term_init)  | Takes over the screen. Only one context exists; calling twice returns it. |
| `void` | [`term_shutdown`](#term_shutdown)  | Gives the screen back to the OS. |
| `int` | [`term_run`](#term_run)  | Runs the draw + input loop. Blocks until [term_quit()](#term_quit); returns its result. |
| `void` | [`term_quit`](#term_quit)  | Ends [term_run()](#term_run) after the current event; it returns `result`. |
| `void` | [`term_set_tick`](#term_set_tick)  | Deliver TERM_EV_TICK every `ms` milliseconds (0 turns ticks off). |

---

### term_init

```cpp
term_ctx_t * term_init(void)
```

Defined in src/titrm.h:216

Takes over the screen. Only one context exists; calling twice returns it.

---

### term_shutdown

```cpp
void term_shutdown(term_ctx_t * ctx)
```

Defined in src/titrm.h:219

Gives the screen back to the OS.

---

### term_run

```cpp
int term_run(term_ctx_t * ctx, term_update_fn update, void * state)
```

Defined in src/titrm.h:227

Runs the draw + input loop. Blocks until [term_quit()](#term_quit); returns its result.

`update` is the global event handler. The screen is redrawn whenever something on it changed.

---

### term_quit

```cpp
void term_quit(term_ctx_t * ctx, int result)
```

Defined in src/titrm.h:230

Ends [term_run()](#term_run) after the current event; it returns `result`.

---

### term_set_tick

```cpp
void term_set_tick(term_ctx_t * ctx, unsigned ms)
```

Defined in src/titrm.h:233

Deliver TERM_EV_TICK every `ms` milliseconds (0 turns ticks off).

