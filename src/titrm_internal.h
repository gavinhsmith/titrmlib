#ifndef TITRM_INTERNAL_H
#define TITRM_INTERNAL_H

#include "titrm.h"
#include "titrm_font.h"

typedef enum {
    TERM_KIND_PLAIN,
    TERM_KIND_TEXT,
    TERM_KIND_LIST,
    TERM_KIND_INPUT,
    TERM_KIND_LOG,
    TERM_KIND_PROGRESS
} term_kind_t;

struct term_panel {
    term_ctx_t *ctx;
    term_panel_t *parent;
    term_panel_t *first_child;
    term_panel_t *last_child;
    term_panel_t *next; /* next sibling */

    uint8_t in_use;
    uint8_t visible;
    uint8_t focusable;
    uint8_t border;
    uint8_t wrap;
    uint8_t dir;       /* how children are arranged, once there are any */
    term_size_t size;  /* how this panel is sized within its parent */
    const char *title;

    /* Outer rectangle and content area (inside the border) in cells, set by
     * layout. */
    uint8_t x, y, w, h;
    uint8_t ix, iy, iw, ih;

    /* Output state, reset at the start of each frame. Inner coordinates. */
    uint8_t cur_x, cur_y;
    uint8_t attr;

    term_draw_fn draw;
    void *draw_user;

    uint8_t kind;
    union {
        struct {
            const char *text;
        } text;
        struct {
            const char *const *items;
            uint16_t count;
            uint16_t sel;
            uint16_t top;
        } list;
        struct {
            char buf[TERM_INPUT_MAX + 1];
            uint8_t len;
            uint8_t cur;
            uint8_t scroll;
        } input;
        struct {
            char *lines; /* ring of `cap` strings, TERM_LOG_LINE bytes each */
            uint16_t cap;
            uint16_t head;  /* next slot to write */
            uint16_t count; /* slots in use */
            uint16_t back;  /* lines scrolled back from the newest */
        } log;
        struct {
            uint16_t value;
            uint16_t max;
        } progress;
    } u;
};

#define TERM_LOG_LINE (TERM_COLS + 1)

struct term_ctx {
    term_panel_t panels[TERM_MAX_PANELS];
    term_panel_t *root;
    term_panel_t *focus;

    term_update_fn update;
    void *state;

    uint8_t layout_dirty;
    uint8_t refocus; /* the focused panel was destroyed: pick a new one */
    uint8_t quit;
    int result;

    uint8_t alpha;  /* 0 off, 1 once, 2 lock */
    uint8_t second; /* previous key was [2nd] */

    unsigned long tick;      /* clock() ticks between TERM_EV_TICK, 0 = off */
    unsigned long last_tick;
};

/* Writes one cell of the current frame, clipped to `p`'s content area. */
void term_put(term_panel_t *p, int col, int row, uint8_t ch, uint8_t attr);

/* Delivers a widget event to the app's update callback. */
void term_emit(term_ctx_t *ctx, term_event_type_t type, term_panel_t *panel, int value);

/* Widget hooks (titrm_widgets.c). */
void term_widget_draw(term_panel_t *p);
bool term_widget_key(term_panel_t *p, const term_event_t *ev); /* true = consumed */
void term_widget_free(term_panel_t *p);

#endif
