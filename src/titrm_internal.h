#ifndef TITRM_INTERNAL_H
#define TITRM_INTERNAL_H

#include "titrm.h"
#include "titrm_font.h"

/* One character cell: glyph code and palette colors. Panels keep their own
 * cells (retained output); each frame they are composed into the screen grid. */
typedef struct {
    uint8_t ch;
    uint8_t fg;
    uint8_t bg;
} term_cell_t;

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

    /* Scene roots (panels with no parent) only: the scene's event handler. */
    term_update_fn handler;
    void *handler_state;

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

    /* Retained content: iw * ih cells, row by row. Only leaf panels have
     * cells; panels with children just lay them out. NULL when empty. */
    term_cell_t *cells;
    uint8_t cells_w, cells_h; /* size of `cells`, kept in step with iw, ih */
    uint8_t stale;            /* widget content must be rebuilt before the next frame */

    /* Output cursor and attribute, in content coordinates. They persist. */
    uint8_t cur_x, cur_y;
    uint8_t attr;

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
#define TERM_KEY_QUEUE 16

struct term_ctx {
    term_panel_t panels[TERM_MAX_PANELS];
    term_panel_t *root;  /* the first scene, from term_init() */
    term_panel_t *scene; /* the active scene */
    term_panel_t *focus;

    term_update_fn update; /* the global handler */
    void *state;
    uint8_t running;       /* inside term_run() */

    uint8_t layout_dirty;
    uint8_t dirty;   /* something changed: the next frame must compose */
    uint8_t focus_lost; /* the focused panel was destroyed: tell the app */
    uint8_t quit;
    int result;

    uint8_t alpha;  /* 0 off, 1 once, 2 lock */
    uint8_t second; /* previous key was [2nd] */

    unsigned long tick;      /* clock() ticks between TERM_EV_TICK, 0 = off */
    unsigned long last_tick;

    /* Scan codes read while a frame was being drawn, handled in order. */
    uint8_t keys[TERM_KEY_QUEUE];
    uint8_t key_head;
    uint8_t key_count;

    /* Keypad state from the last scan (keypadc groups 1-7), and the held key
     * being repeated, if any. */
    uint8_t kb_prev[8];
    uint8_t repeat_key;
    unsigned long repeat_since; /* clock() of its last press or repeat */
    unsigned long repeat_wait;  /* clock() ticks until the next repeat */
};

/* Writes one cell of `p`'s retained content, clipped to its content area. */
void term_put(term_panel_t *p, int col, int row, uint8_t ch, uint8_t attr);

/* Marks a widget's content out of date: it is rebuilt before the next frame. */
void term_panel_touch(term_panel_t *p);

/* Number of scan codes waiting in the key queue (used by the host tests'
 * keypad stub to know when the run is really idle). */
int term_keys_pending(void);

/* Sends an event along the handler chain: the active scene's handler, then
 * the global one, stopping at the first that returns true. */
void term_emit(term_ctx_t *ctx, term_event_type_t type, term_panel_t *panel, int value);

/* Widget hooks (titrm_widgets.c). */
void term_widget_draw(term_panel_t *p);
bool term_widget_key(term_panel_t *p, const term_event_t *ev); /* true = consumed */
void term_widget_free(term_panel_t *p);

#endif
