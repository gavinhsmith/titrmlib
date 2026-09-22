/*
 * titrmlib - TUI framework for the TI-84 Plus CE.
 *
 * titrmlib owns the whole display: it renders a character grid, reads the
 * keypad and drives the event loop. An app builds a tree of panels, gives
 * them content, and hands control to term_run(). It never calls graphx.
 *
 *     term_ctx_t *ctx = term_init();
 *     term_panel_t *list = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
 *     term_make_list(list, items, n);
 *     term_run(ctx, on_event, NULL);
 *     term_shutdown(ctx);
 */
#ifndef TITRM_H
#define TITRM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "titrm_chars.h" /* TERM_CH_* / TERM_S_* glyph names for 0x80-0xFF */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct term_ctx term_ctx_t;
typedef struct term_panel term_panel_t;

/* ---- Limits ------------------------------------------------------------- */

#define TERM_MAX_PANELS 32
#define TERM_INPUT_MAX  48 /* longest text an input widget can hold */

/* ---- Grid --------------------------------------------------------------- */

/* Grid size in cells: 53 x 30 with the built-in 5x7 font (6x8 cells). */
int term_cols(void);
int term_rows(void);

/* Cell attributes for term_panel_set_attr(). */
#define TERM_ATTR_NORMAL  0
#define TERM_ATTR_REVERSE 1 /* inverse video: the only "style" for now */

/* ---- Input -------------------------------------------------------------- */

typedef enum {
    TERM_KEY_NONE = 0,
    TERM_KEY_UP,
    TERM_KEY_DOWN,
    TERM_KEY_LEFT,
    TERM_KEY_RIGHT,
    TERM_KEY_ENTER,
    TERM_KEY_CLEAR,
    TERM_KEY_DEL,
    TERM_KEY_2ND,
    TERM_KEY_MODE,
    TERM_KEY_TAB, /* [vars]: handled by the framework as "next panel" */
    TERM_KEY_F1,  /* [y=] [window] [zoom] [trace] [graph] */
    TERM_KEY_F2,
    TERM_KEY_F3,
    TERM_KEY_F4,
    TERM_KEY_F5,
    TERM_KEY_CHAR /* a printable character; see term_event_t.ch */
} term_key_t;

typedef enum {
    TERM_EV_START,  /* once, before the first frame */
    TERM_EV_KEY,    /* a key the focused panel did not consume */
    TERM_EV_TICK,   /* the interval set with term_set_tick() elapsed */
    TERM_EV_SELECT, /* list item chosen with [enter]; panel = list, value = index */
    TERM_EV_SUBMIT  /* input submitted with [enter]; panel = input */
} term_event_type_t;

typedef struct {
    term_event_type_t type;
    term_key_t key;      /* TERM_EV_KEY */
    char ch;             /* TERM_EV_KEY with TERM_KEY_CHAR: typed character */
    term_panel_t *panel; /* widget events: the source. key events: focused panel */
    int value;           /* TERM_EV_SELECT: item index */
} term_event_t;

/* Called for every event the framework does not handle itself. */
typedef void (*term_update_fn)(term_ctx_t *ctx, const term_event_t *ev, void *state);

/* Alpha state, for status displays: 0 = off, 1 = next key only, 2 = locked.
 * [alpha] arms it, [2nd][alpha] locks it. Letters come out upper case. */
int term_alpha_mode(const term_ctx_t *ctx);

/* ---- Lifecycle ---------------------------------------------------------- */

/* Takes over the screen. Only one context exists; calling twice returns it. */
term_ctx_t *term_init(void);

/* Gives the screen back to the OS. */
void term_shutdown(term_ctx_t *ctx);

/* Runs the draw + input loop. Blocks until term_quit(); returns its result.
 * The screen is redrawn after every key press and tick. */
int term_run(term_ctx_t *ctx, term_update_fn update, void *state);
void term_quit(term_ctx_t *ctx, int result);

/* Deliver TERM_EV_TICK every `ms` milliseconds (0 turns ticks off). */
void term_set_tick(term_ctx_t *ctx, unsigned ms);

/* ---- Panel tree --------------------------------------------------------- */

typedef enum {
    TERM_HORIZONTAL, /* children sit side by side, left to right */
    TERM_VERTICAL    /* children are stacked, top to bottom */
} term_dir_t;

typedef struct {
    uint8_t kind;
    uint8_t value;
} term_size_t;

enum { TERM_SIZE_FIXED, TERM_SIZE_PERCENT, TERM_SIZE_FILL };

#define TERM_FIXED(cells)     ((term_size_t){TERM_SIZE_FIXED, (cells)}) /* n cells */
#define TERM_PERCENT(pct)     ((term_size_t){TERM_SIZE_PERCENT, (pct)}) /* % of parent */
#define TERM_FILL             ((term_size_t){TERM_SIZE_FILL, 1})        /* share the rest */
#define TERM_FILL_WEIGHT(w)   ((term_size_t){TERM_SIZE_FILL, (w)})      /* weighted share */

/* The root panel covers the whole grid. */
term_panel_t *term_root(term_ctx_t *ctx);

/* Adds a child to `parent` and returns it. The first split makes `parent` a
 * container that lays its children out along `dir`; later splits append more
 * children and must use the same `dir` (else NULL is returned, as it is when
 * the panel pool is exhausted). A container's own content is not drawn, apart
 * from its border. Fixed and percent sizes are taken first, then fill panels
 * divide what remains by weight; children that don't fit are clipped. */
term_panel_t *term_split(term_panel_t *parent, term_dir_t dir, term_size_t size);

/* Removes a panel and everything below it. Panel handles become invalid. */
void term_panel_destroy(term_panel_t *panel);

/* Hidden panels take no space; siblings reflow. Takes effect next frame. */
void term_panel_show(term_panel_t *panel, bool visible);
bool term_panel_visible(const term_panel_t *panel);

/* Draws a box around the panel; its content area shrinks by one cell. A title
 * (may be NULL, must outlive the panel) is set into the top edge. */
void term_panel_set_border(term_panel_t *panel, bool border);
void term_panel_set_title(term_panel_t *panel, const char *title);

/* Content size in cells (inside the border), as of the last layout. */
int term_panel_width(const term_panel_t *panel);
int term_panel_height(const term_panel_t *panel);

/* ---- Focus -------------------------------------------------------------- */

/* Widgets that take input (list, input, log) are focusable by default;
 * any panel can opt in. [vars] moves focus forward in tree order. */
void term_panel_set_focusable(term_panel_t *panel, bool focusable);
void term_focus(term_ctx_t *ctx, term_panel_t *panel);
void term_focus_next(term_ctx_t *ctx);
term_panel_t *term_focused(const term_ctx_t *ctx);

/* ---- Panel-scoped output ------------------------------------------------ */
/*
 * Everything below is clipped to the panel's content area, whatever its depth
 * in the tree. Panels are redrawn from scratch every frame: put output in a
 * draw callback rather than expecting it to persist.
 */

typedef void (*term_draw_fn)(term_ctx_t *ctx, term_panel_t *panel, void *user);

/* Called each frame with a blank panel, its cursor at 0,0. */
void term_panel_set_draw(term_panel_t *panel, term_draw_fn draw, void *user);

void term_panel_move(term_panel_t *panel, int col, int row);
void term_panel_set_attr(term_panel_t *panel, uint8_t attr);
void term_panel_wrap(term_panel_t *panel, bool wrap); /* wrap at the right edge (default: clip) */
void term_panel_putc(term_panel_t *panel, char c);    /* '\n' starts a new line */
void term_panel_print(term_panel_t *panel, const char *str);
void term_panel_printf(term_panel_t *panel, const char *fmt, ...);
void term_panel_repeat(term_panel_t *panel, char c, int count);
void term_panel_clear(term_panel_t *panel);

/* ---- Widgets ------------------------------------------------------------ */
/* term_make_* turns a panel into a widget; the term_<widget>_* calls drive it. */

/* Static text, word-wrapped to the panel. `text` must outlive the panel. */
void term_make_text(term_panel_t *panel, const char *text);
void term_text_set(term_panel_t *panel, const char *text);

/* Selectable list. up/down move, [enter] emits TERM_EV_SELECT. Items are not
 * copied. Embed icons with TERM_S_* (e.g. TERM_S_CHECK "Done"). */
void term_make_list(term_panel_t *panel, const char *const *items, int count);
void term_list_set_items(term_panel_t *panel, const char *const *items, int count);
int term_list_selected(const term_panel_t *panel);
void term_list_select(term_panel_t *panel, int index);

/* Single-line text field. Typing inserts, [del] backspaces, [clear] empties,
 * left/right move the cursor, [enter] emits TERM_EV_SUBMIT. */
void term_make_input(term_panel_t *panel);
const char *term_input_text(const term_panel_t *panel);
void term_input_set(term_panel_t *panel, const char *text);

/* Scrollback. New lines are appended at the bottom; up/down scroll back. */
void term_make_log(term_panel_t *panel, int max_lines);
void term_log_print(term_panel_t *panel, const char *str); /* may contain '\n' */
void term_log_printf(term_panel_t *panel, const char *fmt, ...);
void term_log_clear(term_panel_t *panel);

/* Horizontal progress bar filling the panel's first row. */
void term_make_progress(term_panel_t *panel, int max);
void term_progress_set(term_panel_t *panel, int value);

#ifdef __cplusplus
}
#endif

#endif
