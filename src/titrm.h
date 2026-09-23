/**
 * @file titrm.h
 * @brief titrmlib - TUI framework for the TI-84 Plus CE.
 *
 * titrmlib owns the whole display: it renders a character grid, reads the
 * keypad and drives the event loop. An app builds a tree of panels, gives
 * them content, and hands control to term_run(). It never calls graphx.
 *
 * @code
 * term_ctx_t *ctx = term_init();
 * term_panel_t *list = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
 * term_make_list(list, items, n);
 * term_run(ctx, on_event, NULL);
 * term_shutdown(ctx);
 * @endcode
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

/**
 * @defgroup version Version
 * @{
 */

/** @brief titrmlib's version, matching the release tag without the "v" (e.g. "1.0.0"). */
#define TITRM_VERSION "0.1.0"

/** @} */

/**
 * @defgroup types Handles
 * @brief Opaque handles passed to every call.
 * @{
 */

/** @brief The framework context. There is only one; see term_init(). */
typedef struct term_ctx term_ctx_t;

/** @brief A panel in the tree. Handles stay valid until the panel is destroyed. */
typedef struct term_panel term_panel_t;

/** @} */

/**
 * @defgroup limits Limits
 * @{
 */

/** @brief Panels available, including the root. */
#define TERM_MAX_PANELS 32

/** @brief Longest text an input widget can hold. */
#define TERM_INPUT_MAX  48

/** @} */

/**
 * @defgroup grid Grid
 * @brief The character grid and cell attributes.
 * @{
 */

/** @brief Grid width in cells: 53 with the built-in 5x7 font (6x8 cells). */
int term_cols(void);

/** @brief Grid height in cells: 30 with the built-in 5x7 font (6x8 cells). */
int term_rows(void);

/** @brief Cell attribute for term_panel_set_attr(): normal video. */
#define TERM_ATTR_NORMAL  0

/** @brief Cell attribute for term_panel_set_attr(): inverse video, the only "style" for now. */
#define TERM_ATTR_REVERSE 1

/** @} */

/**
 * @defgroup input Input
 * @brief Keys and the events delivered to the app.
 * @{
 */

/** @brief Keys, as delivered in term_event_t.key. */
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
    TERM_KEY_TAB, /**< [vars]: handled by the framework as "next panel" */
    TERM_KEY_F1,  /**< [y=]; F2-F5 are [window] [zoom] [trace] [graph] */
    TERM_KEY_F2,
    TERM_KEY_F3,
    TERM_KEY_F4,
    TERM_KEY_F5,
    TERM_KEY_CHAR /**< a printable character; see term_event_t.ch */
} term_key_t;

/** @brief Kinds of event passed to the update function. */
typedef enum {
    TERM_EV_START,  /**< once, before the first frame */
    TERM_EV_KEY,    /**< a key the focused panel did not consume */
    TERM_EV_TICK,   /**< the interval set with term_set_tick() elapsed */
    TERM_EV_SELECT, /**< list item chosen with [enter]; panel = list, value = index */
    TERM_EV_SUBMIT  /**< input submitted with [enter]; panel = input */
} term_event_type_t;

/** @brief An event passed to the update function. */
typedef struct {
    term_event_type_t type; /**< what happened */
    term_key_t key;      /**< TERM_EV_KEY */
    char ch;             /**< TERM_EV_KEY with TERM_KEY_CHAR: typed character */
    term_panel_t *panel; /**< widget events: the source. key events: focused panel */
    int value;           /**< TERM_EV_SELECT: item index */
} term_event_t;

/** @brief Called for every event the framework does not handle itself. */
typedef void (*term_update_fn)(term_ctx_t *ctx, const term_event_t *ev, void *state);

/**
 * @brief Alpha state, for status displays: 0 = off, 1 = next key only, 2 = locked.
 *
 * [alpha] arms it, [2nd][alpha] locks it. Letters come out upper case.
 */
int term_alpha_mode(const term_ctx_t *ctx);

/** @} */

/**
 * @defgroup lifecycle Lifecycle
 * @brief Starting, running and stopping the framework.
 * @{
 */

/** @brief Takes over the screen. Only one context exists; calling twice returns it. */
term_ctx_t *term_init(void);

/** @brief Gives the screen back to the OS. */
void term_shutdown(term_ctx_t *ctx);

/**
 * @brief Runs the draw + input loop. Blocks until term_quit(); returns its result.
 *
 * The screen is redrawn after every key press and tick.
 */
int term_run(term_ctx_t *ctx, term_update_fn update, void *state);

/** @brief Ends term_run() after the current event; it returns `result`. */
void term_quit(term_ctx_t *ctx, int result);

/** @brief Deliver TERM_EV_TICK every `ms` milliseconds (0 turns ticks off). */
void term_set_tick(term_ctx_t *ctx, unsigned ms);

/** @} */

/**
 * @defgroup panels Panel tree
 * @brief Splitting the screen into panels and sizing them.
 * @{
 */

/** @brief How a panel lays out its children. */
typedef enum {
    TERM_HORIZONTAL, /**< children sit side by side, left to right */
    TERM_VERTICAL    /**< children are stacked, top to bottom */
} term_dir_t;

/** @brief A panel's size within its parent. Build one with TERM_FIXED, TERM_PERCENT, TERM_FILL or TERM_FILL_WEIGHT. */
typedef struct {
    uint8_t kind;  /**< TERM_SIZE_FIXED, TERM_SIZE_PERCENT or TERM_SIZE_FILL */
    uint8_t value; /**< cells, percent or fill weight */
} term_size_t;

/** @brief Values of term_size_t.kind. */
enum term_size_kind { TERM_SIZE_FIXED, TERM_SIZE_PERCENT, TERM_SIZE_FILL };

/** @brief Exactly `cells` cells. */
#define TERM_FIXED(cells)     ((term_size_t){TERM_SIZE_FIXED, (cells)})
/** @brief `pct` percent of the parent. */
#define TERM_PERCENT(pct)     ((term_size_t){TERM_SIZE_PERCENT, (pct)})
/** @brief An equal share of the space left after fixed and percent siblings. */
#define TERM_FILL             ((term_size_t){TERM_SIZE_FILL, 1})
/** @brief A share of the space left, weighted by `w` against other fill siblings. */
#define TERM_FILL_WEIGHT(w)   ((term_size_t){TERM_SIZE_FILL, (w)})

/** @brief The root panel covers the whole grid. */
term_panel_t *term_root(term_ctx_t *ctx);

/**
 * @brief Adds a child to `parent` and returns it.
 *
 * The first split makes `parent` a container that lays its children out
 * along `dir`; later splits append more children and must use the same `dir`
 * (else NULL is returned, as it is when the panel pool is exhausted). A
 * container's own content is not drawn, apart from its border. Fixed and
 * percent sizes are taken first, then fill panels divide what remains by
 * weight; children that don't fit are clipped.
 */
term_panel_t *term_split(term_panel_t *parent, term_dir_t dir, term_size_t size);

/** @brief Removes a panel and everything below it. Panel handles become invalid. */
void term_panel_destroy(term_panel_t *panel);

/** @brief Shows or hides a panel. Hidden panels take no space; siblings reflow. Takes effect next frame. */
void term_panel_show(term_panel_t *panel, bool visible);

/** @brief Whether the panel is shown (see term_panel_show()). */
bool term_panel_visible(const term_panel_t *panel);

/** @brief Draws a box around the panel; its content area shrinks by one cell. */
void term_panel_set_border(term_panel_t *panel, bool border);

/** @brief Sets a title into the top edge of the border. May be NULL; must outlive the panel. */
void term_panel_set_title(term_panel_t *panel, const char *title);

/** @brief Content width in cells (inside the border), as of the last layout. */
int term_panel_width(const term_panel_t *panel);

/** @brief Content height in cells (inside the border), as of the last layout. */
int term_panel_height(const term_panel_t *panel);

/** @} */

/**
 * @defgroup focus Focus
 * @brief Which panel receives keys.
 *
 * Widgets that take input (list, input, log) are focusable by default; any
 * panel can opt in. [vars] moves focus forward in tree order.
 * @{
 */

/** @brief Lets a panel take focus, or stops it. */
void term_panel_set_focusable(term_panel_t *panel, bool focusable);

/** @brief Moves focus to `panel` (making it focusable), or clears it with NULL. */
void term_focus(term_ctx_t *ctx, term_panel_t *panel);

/** @brief Moves focus to the next focusable panel in tree order, as [vars] does. */
void term_focus_next(term_ctx_t *ctx);

/** @brief The focused panel, or NULL. */
term_panel_t *term_focused(const term_ctx_t *ctx);

/** @} */

/**
 * @defgroup output Panel-scoped output
 * @brief Drawing text into a panel.
 *
 * Everything here is clipped to the panel's content area, whatever its depth
 * in the tree. Panels are redrawn from scratch every frame: put output in a
 * draw callback rather than expecting it to persist.
 * @{
 */

/** @brief A draw callback; see term_panel_set_draw(). */
typedef void (*term_draw_fn)(term_ctx_t *ctx, term_panel_t *panel, void *user);

/** @brief Sets the panel's draw callback, called each frame with a blank panel, its cursor at 0,0. */
void term_panel_set_draw(term_panel_t *panel, term_draw_fn draw, void *user);

/** @brief Moves the panel's cursor. */
void term_panel_move(term_panel_t *panel, int col, int row);

/** @brief Sets the attribute (TERM_ATTR_*) for the text printed next. */
void term_panel_set_attr(term_panel_t *panel, uint8_t attr);

/** @brief Wrap at the right edge instead of clipping (the default). */
void term_panel_wrap(term_panel_t *panel, bool wrap);

/** @brief Prints one character. '\\n' starts a new line. */
void term_panel_putc(term_panel_t *panel, char c);

/** @brief Prints a string at the cursor. */
void term_panel_print(term_panel_t *panel, const char *str);

/** @brief Prints formatted text at the cursor. */
void term_panel_printf(term_panel_t *panel, const char *fmt, ...);

/** @brief Prints `c` `count` times. */
void term_panel_repeat(term_panel_t *panel, char c, int count);

/** @brief Blanks the panel's content area and moves the cursor to 0,0. */
void term_panel_clear(term_panel_t *panel);

/** @} */

/**
 * @defgroup widgets Widgets
 * @brief Panels with built-in content and key handling.
 *
 * term_make_* turns a panel into a widget; the term_<widget>_* calls drive it.
 * @{
 */

/** @brief Static text, word-wrapped to the panel. `text` must outlive the panel. */
void term_make_text(term_panel_t *panel, const char *text);

/** @brief Replaces a text widget's text. `text` must outlive the panel. */
void term_text_set(term_panel_t *panel, const char *text);

/**
 * @brief Selectable list. up/down move, [enter] emits TERM_EV_SELECT.
 *
 * Items are not copied. Embed icons with TERM_S_* (e.g. TERM_S_CHECK "Done").
 */
void term_make_list(term_panel_t *panel, const char *const *items, int count);

/** @brief Replaces a list's items. Items are not copied. */
void term_list_set_items(term_panel_t *panel, const char *const *items, int count);

/** @brief Index of the selected item, or -1 if the list is empty. */
int term_list_selected(const term_panel_t *panel);

/** @brief Selects an item; out-of-range indexes are clamped. */
void term_list_select(term_panel_t *panel, int index);

/**
 * @brief Single-line text field.
 *
 * Typing inserts, [del] backspaces, [clear] empties, left/right move the
 * cursor, [enter] emits TERM_EV_SUBMIT.
 */
void term_make_input(term_panel_t *panel);

/** @brief The input's current text. */
const char *term_input_text(const term_panel_t *panel);

/** @brief Replaces the input's text (up to TERM_INPUT_MAX characters). */
void term_input_set(term_panel_t *panel, const char *text);

/** @brief Scrollback holding up to `max_lines`. New lines are appended at the bottom; up/down scroll back. */
void term_make_log(term_panel_t *panel, int max_lines);

/** @brief Appends text to a log; each '\\n' starts a new line. */
void term_log_print(term_panel_t *panel, const char *str);

/** @brief Appends formatted text to a log. */
void term_log_printf(term_panel_t *panel, const char *fmt, ...);

/** @brief Removes every line from a log. */
void term_log_clear(term_panel_t *panel);

/** @brief Horizontal progress bar filling the panel's first row, from 0 to `max`. */
void term_make_progress(term_panel_t *panel, int max);

/** @brief Sets the progress value; it is clamped to 0..max. */
void term_progress_set(term_panel_t *panel, int value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
