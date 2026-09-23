

# Input

Keys and the events delivered to the app.

## Classes

| Name | Description |
|------|-------------|
| [`term_event_t`](#term_event_t) | An event passed to the update function. |

## Enumerations

| Name | Description |
|------|-------------|
| [`term_key_t`](#term_key_t)  | Keys, as delivered in [term_event_t.key](#key). |
| [`term_event_type_t`](#term_event_type_t)  | Kinds of event passed to the update function. |

---

### term_key_t

```cpp
enum term_key_t
```

Defined in src/titrm.h:84

Keys, as delivered in [term_event_t.key](#key).

| Value | Description |
|-------|-------------|
| `TERM_KEY_NONE` |  |
| `TERM_KEY_UP` |  |
| `TERM_KEY_DOWN` |  |
| `TERM_KEY_LEFT` |  |
| `TERM_KEY_RIGHT` |  |
| `TERM_KEY_ENTER` |  |
| `TERM_KEY_CLEAR` |  |
| `TERM_KEY_DEL` |  |
| `TERM_KEY_2ND` |  |
| `TERM_KEY_MODE` |  |
| `TERM_KEY_TAB` | [vars]: handled by the framework as "next panel" |
| `TERM_KEY_F1` | [y=]; F2-F5 are [window] [zoom] [trace] [graph] |
| `TERM_KEY_F2` |  |
| `TERM_KEY_F3` |  |
| `TERM_KEY_F4` |  |
| `TERM_KEY_F5` |  |
| `TERM_KEY_CHAR` | a printable character; see [term_event_t.ch](#ch) |

---

### term_event_type_t

```cpp
enum term_event_type_t
```

Defined in src/titrm.h:105

Kinds of event passed to the update function.

| Value | Description |
|-------|-------------|
| `TERM_EV_START` | once, before the first frame |
| `TERM_EV_KEY` | a key the focused panel did not consume |
| `TERM_EV_TICK` | the interval set with [term_set_tick()](api-lifecycle.md#term_set_tick) elapsed |
| `TERM_EV_SELECT` | list item chosen with [enter]; panel = list, value = index |
| `TERM_EV_SUBMIT` | input submitted with [enter]; panel = input |
## Typedefs

| Return | Name | Description |
|--------|------|-------------|
| `void(*)` | [`term_update_fn`](#term_update_fn)  | Called for every event the framework does not handle itself. |

---

### term_update_fn

```cpp
using term_update_fn = void(*)
```

Defined in src/titrm.h:123

Called for every event the framework does not handle itself.

## Functions

| Return | Name | Description |
|--------|------|-------------|
| `int` | [`term_alpha_mode`](#term_alpha_mode)  | Alpha state, for status displays: 0 = off, 1 = next key only, 2 = locked. |

---

### term_alpha_mode

```cpp
int term_alpha_mode(const term_ctx_t * ctx)
```

Defined in src/titrm.h:130

Alpha state, for status displays: 0 = off, 1 = next key only, 2 = locked.

[alpha] arms it, [2nd][alpha] locks it. Letters come out upper case.


## Class Definitions



### term_event_t

```cpp
#include <titrm.h>
```

```cpp
struct term_event_t
```

Defined in src/titrm.h:114

An event passed to the update function.

#### Public Attributes

| Return | Name | Description |
|--------|------|-------------|
| [`term_event_type_t`](#term_event_type_t) | [`type`](#type)  | what happened |
| [`term_key_t`](#term_key_t) | [`key`](#key)  | TERM_EV_KEY |
| `char` | [`ch`](#ch)  | TERM_EV_KEY with TERM_KEY_CHAR: typed character |
| [`term_panel_t`](api-types.md#term_panel_t) * | [`panel`](#panel)  | widget events: the source. key events: focused panel |
| `int` | [`value`](#value)  | TERM_EV_SELECT: item index |

---

##### type

```cpp
term_event_type_t type
```

Type: [`term_event_type_t`](#term_event_type_t)

Defined in src/titrm.h:115

what happened

---

##### key

```cpp
term_key_t key
```

Type: [`term_key_t`](#term_key_t)

Defined in src/titrm.h:116

TERM_EV_KEY

---

##### ch

```cpp
char ch
```

Defined in src/titrm.h:117

TERM_EV_KEY with TERM_KEY_CHAR: typed character

---

##### panel

```cpp
term_panel_t * panel
```

Type: [`term_panel_t`](api-types.md#term_panel_t) *

Defined in src/titrm.h:118

widget events: the source. key events: focused panel

---

##### value

```cpp
int value
```

Defined in src/titrm.h:119

TERM_EV_SELECT: item index

