#include "titrm_internal.h"

#include <graphx.h>
#include <keypadc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ti/getcsc.h>
#include <ti/screen.h>

/* Default colors: white on black. */
#define TERM_FG TERM_COLOR_WHITE
#define TERM_BG TERM_COLOR_BLACK

/* The grid doesn't fill the screen exactly; centre it. */
#define ORIGIN_X ((TERM_SCREEN_W - TERM_COLS * TERM_CELL_W) / 2)
#define ORIGIN_Y ((TERM_SCREEN_H - TERM_ROWS * TERM_CELL_H) / 2)

static term_cell_t grid[TERM_ROWS][TERM_COLS];  /* the frame being composed */
static term_cell_t *grid_row[TERM_ROWS];        /* &grid[r][0]: indexing grid costs a multiply call */
static term_cell_t shown[TERM_ROWS][TERM_COLS]; /* what is on the screen */
static term_cell_t blank_row[TERM_COLS];        /* empty cells, copied to clear */

static const term_cell_t blank_cell = {0, TERM_FG, TERM_BG, 0};

static term_ctx_t g_ctx;
static bool g_open;

int term_cols(void) { return TERM_COLS; }
int term_rows(void) { return TERM_ROWS; }

/* Sets a cell in the panel's colors; TERM_ATTR_REVERSE swaps them. Cells are
 * written field by field: clang builds a 4-byte struct returned by value as
 * a 32-bit integer, with a library call per byte. */
static inline __attribute__((always_inline)) void set_cell(term_cell_t *c, const term_panel_t *p,
                                                        uint8_t ch, uint8_t attr) {
    bool reverse = attr & TERM_ATTR_REVERSE;
    c->ch = ch;
    c->fg = reverse ? p->bg : p->fg;
    c->bg = reverse ? p->fg : p->bg;
    c->style = attr & TERM_STYLES;
}

static void set_blank(term_cell_t *c, const term_panel_t *p) {
    set_cell(c, p, 0, TERM_ATTR_NORMAL);
}

/* ---- Key queue ----------------------------------------------------------- */

/* The keypad is read with keypadc rather than os_GetCSC(): a scan costs
 * about 1.3 ms against about 19 ms for os_GetCSC(), which matters because it
 * runs on every loop and between rows while a frame is drawn. Presses are
 * found by comparing each scan with the last, and held keys are repeated here
 * since the OS no longer does it. Keys are named by their os_GetCSC() scan
 * codes, which follow the keypad's group and bit: (7 - group) * 8 + bit + 1. */

#define REPEAT_DELAY (CLOCKS_PER_SEC * 2 / 5) /* 400 ms before a held key repeats */
#define REPEAT_RATE  (CLOCKS_PER_SEC / 12)    /* then about 12 times a second */

static void queue_key(term_ctx_t *ctx, uint8_t sk) {
    if (ctx->key_count < TERM_KEY_QUEUE) {
        ctx->keys[(ctx->key_head + ctx->key_count) % TERM_KEY_QUEUE] = sk;
        ctx->key_count++;
    }
}

/* Like the OS, only the arrows and [del] repeat. */
static bool repeats(uint8_t sk) {
    return (sk >= sk_Down && sk <= sk_Up) || sk == sk_Del;
}

static bool held(const term_ctx_t *ctx, uint8_t sk) {
    return ctx->kb_prev[7 - ((sk - 1) >> 3)] & (1 << ((sk - 1) & 7));
}

/* Reads the keypad into the queue. Called by the run loop and between rows
 * while a frame is drawn, so keys pressed during a slow frame aren't lost. */
static void poll_keys(term_ctx_t *ctx) {
    kb_Scan();
    clock_t now = clock();
    for (uint8_t group = 1; group <= 7; group++) {
        uint8_t down = kb_Data[group];
        uint8_t pressed = down & ~ctx->kb_prev[group];
        ctx->kb_prev[group] = down;
        for (uint8_t bit = 0; pressed; bit++, pressed >>= 1) {
            if (pressed & 1) {
                uint8_t sk = ((7 - group) << 3) + bit + 1;
                queue_key(ctx, sk);
                if (repeats(sk)) {
                    ctx->repeat_key = sk;
                    ctx->repeat_since = now;
                    ctx->repeat_wait = REPEAT_DELAY;
                }
            }
        }
    }
    if (ctx->repeat_key && !held(ctx, ctx->repeat_key)) {
        ctx->repeat_key = 0;
    } else if (ctx->repeat_key && now - ctx->repeat_since >= ctx->repeat_wait) {
        queue_key(ctx, ctx->repeat_key);
        ctx->repeat_since = now;
        ctx->repeat_wait = REPEAT_RATE;
    }
}

/* Keys already down when titrmlib starts (like the [enter] that launched the
 * program) are not presses. */
static void init_keys(term_ctx_t *ctx) {
    kb_Scan();
    for (uint8_t group = 1; group <= 7; group++) {
        ctx->kb_prev[group] = kb_Data[group];
    }
}

static uint8_t next_key(term_ctx_t *ctx) {
    if (!ctx->key_count) {
        return 0;
    }
    uint8_t sk = ctx->keys[ctx->key_head];
    ctx->key_head = (ctx->key_head + 1) % TERM_KEY_QUEUE;
    ctx->key_count--;
    return sk;
}

int term_keys_pending(void) {
    return g_ctx.key_count;
}

/* ---- Glyph blit ---------------------------------------------------------- */

/* Box drawing and the blocks are stretched over the gap between cells
 * (right column and bottom row are repeated) so that lines join up. */
static bool is_connected(uint8_t ch) {
    return ch >= TERM_CH_CONNECTED_FIRST && ch <= TERM_CH_CONNECTED_LAST;
}

/* The pixels of one cell row for each 6-bit mask, bit (CELL_W-1) leftmost,
 * per color pair. Slot 0 is white on black: 0xFF where the glyph is set, 0x00
 * elsewhere, from which any other pair is (pixel & (fg ^ bg)) ^ bg. The other
 * slots hold the pairs drawn most recently, replaced in turn. Building a slot
 * costs about as much as drawing a few cells in a per-pixel loop, which a
 * table saves on every cell after that.
 * ponytail: a UI using more than PAIRS - 1 other pairs at once rebuilds
 * tables while drawing; raise PAIRS if that shows up in frame times. */
#define ROW_MASKS (1 << TERM_CELL_W)
#define PAIRS 6
typedef uint8_t row_table_t[ROW_MASKS][TERM_CELL_W];
static row_table_t row_pixels[PAIRS];
static uint8_t pair_fg[PAIRS], pair_bg[PAIRS];
static uint8_t pair_last, pair_next;

static void init_row_pixels(void) {
    for (int m = 0; m < ROW_MASKS; m++) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            row_pixels[0][m][x] = (m >> (TERM_CELL_W - 1 - x) & 1) ? 0xFF : 0x00;
        }
    }
    for (int i = 0; i < PAIRS; i++) {
        pair_fg[i] = 0xFF; /* all white on black until used */
        pair_bg[i] = 0x00;
    }
    pair_last = 0;
    pair_next = 1;
}

static const row_table_t *pixels_for(uint8_t fg, uint8_t bg) {
    if (pair_fg[pair_last] == fg && pair_bg[pair_last] == bg) {
        return &row_pixels[pair_last];
    }
    for (uint8_t i = 0; i < PAIRS; i++) {
        if (pair_fg[i] == fg && pair_bg[i] == bg) {
            pair_last = i;
            return &row_pixels[i];
        }
    }
    uint8_t i = pair_next;
    pair_next = pair_next + 1 < PAIRS ? pair_next + 1 : 1;
    uint8_t diff = fg ^ bg;
    const uint8_t *src = &row_pixels[0][0][0];
    uint8_t *dst = &row_pixels[i][0][0];
    for (int n = 0; n < ROW_MASKS * TERM_CELL_W; n++) {
        dst[n] = (src[n] & diff) ^ bg;
    }
    pair_fg[i] = fg;
    pair_bg[i] = bg;
    pair_last = i;
    return &row_pixels[i];
}

/* Styles, as changes to the glyph's row masks: bold smears each row one
 * pixel right, italic shifts the top rows right, and underline and
 * strikethrough fill a whole row, gap column included, so they join up.
 * ponytail: STRIKE_ROW is a guess at 5x7; pick it on real hardware. */
#define ITALIC_ROWS 3
#define STRIKE_ROW 3
#define FULL_ROW (ROW_MASKS - 1)

static void draw_cell(int col, int row, const term_cell_t *c) {
    const row_table_t *pixels = pixels_for(c->fg, c->bg);
    const term_glyph_t *g = &term_font[c->ch];
    bool blank = (c->ch == 0 || c->ch == ' ');
    bool connected = is_connected(c->ch);
    uint8_t style = connected ? 0 : c->style;

    /* Rows are copied straight into graphx's draw buffer: a graphx call per
     * run of pixels cost about 1 ms a cell. */
    uint8_t *dst = &gfx_vbuffer[ORIGIN_Y + row * TERM_CELL_H][ORIGIN_X + col * TERM_CELL_W];
    for (uint8_t r = 0; r < TERM_CELL_H; r++, dst += TERM_SCREEN_W) {
        uint8_t mask = 0;
        if (!blank && (r < TERM_GLYPH_H || connected)) {
            /* Widen the 5-bit row to a cell-wide mask, bit (CELL_W-1) leftmost.
             * The line gap repeats the last row for connected glyphs. */
            uint8_t bits = g->rows[r < TERM_GLYPH_H ? r : TERM_GLYPH_H - 1];
            mask = bits << TERM_CHAR_GAP;
            if (connected && (bits & 1)) {
                mask |= (1 << TERM_CHAR_GAP) - 1;
            }
        }
        if (style) {
            if ((style & TERM_ATTR_ITALIC) && r < ITALIC_ROWS) {
                mask >>= 1;
            }
            if (style & TERM_ATTR_BOLD) {
                mask |= mask >> 1;
            }
            if (((style & TERM_ATTR_UNDERLINE) && r == TERM_CELL_H - 1) ||
                ((style & TERM_ATTR_STRIKE) && r == STRIKE_ROW)) {
                mask = FULL_ROW;
            }
        }
        memcpy(dst, (*pixels)[mask], TERM_CELL_W);
    }
}

/* Draws only the cells that differ from what is already on screen, reading
 * the keypad after each row it had to redraw. */
static void flush(term_ctx_t *ctx) {
    for (int r = 0; r < TERM_ROWS; r++) {
        if (memcmp(grid[r], shown[r], sizeof grid[r]) == 0) {
            continue;
        }
        term_cell_t *want = grid[r];
        term_cell_t *have = shown[r];
        for (int c = 0; c < TERM_COLS; c++) {
            if (want[c].ch != have[c].ch || want[c].fg != have[c].fg || want[c].bg != have[c].bg ||
                want[c].style != have[c].style) {
                draw_cell(c, r, &want[c]);
                have[c] = want[c];
            }
        }
        poll_keys(ctx);
    }
}

/* ---- Panel tree ---------------------------------------------------------- */

static void mark_dirty(term_ctx_t *ctx) {
    ctx->dirty = 1;
}

static term_panel_t *alloc_panel(term_ctx_t *ctx) {
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        term_panel_t *p = &ctx->panels[i];
        if (!p->in_use) {
            memset(p, 0, sizeof *p);
            p->ctx = ctx;
            p->in_use = 1;
            p->visible = 1;
            p->size = TERM_FILL;
            p->focus_attr = TERM_ATTR_REVERSE;
            p->fg = TERM_FG;
            p->bg = TERM_BG;
            return p;
        }
    }
    return NULL;
}

static void free_cells(term_panel_t *p) {
    free(p->cells);
    p->cells = NULL;
    p->cells_w = 0;
    p->cells_h = 0;
}

term_panel_t *term_root(term_ctx_t *ctx) {
    return ctx->root;
}

term_panel_t *term_split(term_panel_t *parent, term_dir_t dir, term_size_t size) {
    if (!parent || parent->kind != TERM_KIND_PLAIN) {
        return NULL;
    }
    if (parent->first_child && parent->dir != dir) {
        return NULL;
    }
    term_panel_t *child = alloc_panel(parent->ctx);
    if (!child) {
        return NULL;
    }

    parent->dir = dir;
    child->parent = parent;
    child->size = size;
    child->fg = parent->fg;
    child->bg = parent->bg;
    if (parent->last_child) {
        parent->last_child->next = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;

    parent->ctx->layout_dirty = 1;
    return child;
}

static void free_subtree(term_panel_t *p) {
    term_panel_t *c = p->first_child;
    while (c) {
        term_panel_t *next = c->next;
        free_subtree(c);
        c = next;
    }
    if (p->ctx->focus == p) {
        p->ctx->focus = NULL;
        p->ctx->focus_lost = 1; /* reported to the app at the next frame */
    }
    for (int i = 0; i < p->ctx->n_overlays; i++) {
        if (p->ctx->overlays[i]->restore == p) {
            p->ctx->overlays[i]->restore = NULL; /* nothing to give focus back to */
        }
    }
    term_widget_free(p);
    free_cells(p);
    p->in_use = 0;
}

static void remove_overlay(term_ctx_t *ctx, term_panel_t *ov) {
    int j = 0;
    for (int i = 0; i < ctx->n_overlays; i++) {
        if (ctx->overlays[i] != ov) {
            ctx->overlays[j++] = ctx->overlays[i];
        }
    }
    ctx->n_overlays = j;
}

void term_panel_destroy(term_panel_t *p) {
    if (!p) {
        return;
    }
    if (p->owner) {
        term_overlay_close(p);
        return;
    }
    if (p == p->ctx->root || p == p->ctx->scene) {
        return;
    }
    if (!p->parent) { /* a scene that isn't active, and its overlays */
        term_ctx_t *ctx = p->ctx;
        for (int i = ctx->n_overlays - 1; i >= 0; i--) {
            term_panel_t *ov = ctx->overlays[i];
            if (ov->owner == p) {
                remove_overlay(ctx, ov);
                free_subtree(ov);
            }
        }
        free_subtree(p);
        return;
    }

    term_panel_t *parent = p->parent;
    if (parent->first_child == p) {
        parent->first_child = p->next;
        if (parent->last_child == p) {
            parent->last_child = NULL;
        }
    } else {
        term_panel_t *prev = parent->first_child;
        while (prev->next != p) {
            prev = prev->next;
        }
        prev->next = p->next;
        if (parent->last_child == p) {
            parent->last_child = prev;
        }
    }

    p->ctx->layout_dirty = 1;
    free_subtree(p);
}

void term_panel_show(term_panel_t *p, bool visible) {
    p->visible = visible;
    p->ctx->layout_dirty = 1;
}

bool term_panel_visible(const term_panel_t *p) {
    return p->visible;
}

void term_panel_set_border(term_panel_t *p, bool border) {
    p->border = border;
    p->ctx->layout_dirty = 1;
}

void term_panel_set_title(term_panel_t *p, const char *title) {
    p->title = title;
    mark_dirty(p->ctx);
}

/* The content area: the outer rect minus the border. Set by layout, so that
 * term_put(), which runs for every character drawn, only reads it. */
static void set_inner(term_panel_t *p) {
    uint8_t b = p->border ? 1 : 0;
    if (b && (p->w < 2 || p->h < 2)) {
        p->ix = p->x;
        p->iy = p->y;
        p->iw = 0;
        p->ih = 0;
    } else {
        p->ix = p->x + b;
        p->iy = p->y + b;
        p->iw = p->w - 2 * b;
        p->ih = p->h - 2 * b;
    }
}

/* ---- Retained content ---------------------------------------------------- */

/* Gives a leaf panel iw * ih cells, keeping whatever fits of its old content
 * (top-left aligned). Panels with children, or with no area, get none. */
static void size_cells(term_panel_t *p) {
    uint8_t w = p->first_child ? 0 : p->iw;
    uint8_t h = p->first_child ? 0 : p->ih;
    if (w == p->cells_w && h == p->cells_h) {
        return;
    }
    term_cell_t *cells = NULL;
    if (w && h) {
        cells = malloc((size_t)w * h * sizeof *cells);
        if (!cells) {
            w = h = 0; /* out of memory: the panel shows nothing */
        }
    }
    for (int r = 0; r < h; r++) {
        term_cell_t *row = cells + (size_t)r * w;
        for (int c = 0; c < w; c++) {
            if (r < p->cells_h && c < p->cells_w) {
                row[c] = p->cells[(size_t)r * p->cells_w + c];
            } else {
                set_blank(&row[c], p);
            }
        }
    }
    free(p->cells);
    p->cells = cells;
    p->cells_w = w;
    p->cells_h = h;
    p->stale = 1; /* widgets redraw at their new size */
}

void term_panel_touch(term_panel_t *p) {
    p->stale = 1;
    mark_dirty(p->ctx);
}

/* ---- Layout -------------------------------------------------------------- */

/* Lays out `p`'s children inside `p`'s content area, recursively, and sizes
 * each panel's retained cells to match. */
static void layout(term_panel_t *p) {
    set_inner(p);
    size_cells(p);
    if (!p->first_child) {
        return;
    }

    int ix = p->ix;
    int iy = p->iy;
    int iw = p->iw;
    int ih = p->ih;
    bool horiz = (p->dir == TERM_HORIZONTAL);
    int span = horiz ? iw : ih;

    /* Pass 1: fixed and percent sizes come off the top; fill shares the rest. */
    int used = 0;
    int weight = 0;
    for (term_panel_t *c = p->first_child; c; c = c->next) {
        if (!c->visible) {
            continue;
        }
        if (c->size.kind == TERM_SIZE_FIXED) {
            used += c->size.value;
        } else if (c->size.kind == TERM_SIZE_PERCENT) {
            used += span * c->size.value / 100;
        } else {
            weight += c->size.value;
        }
    }
    int spare = span > used ? span - used : 0;

    /* Pass 2: hand out the space. Progressive division gives fill panels an
     * even share with any rounding remainder landing on the last one. */
    int pos = 0;
    for (term_panel_t *c = p->first_child; c; c = c->next) {
        if (!c->visible) {
            continue;
        }
        int n;
        if (c->size.kind == TERM_SIZE_FIXED) {
            n = c->size.value;
        } else if (c->size.kind == TERM_SIZE_PERCENT) {
            n = span * c->size.value / 100;
        } else {
            n = weight ? spare * c->size.value / weight : 0;
            spare -= n;
            weight -= c->size.value;
        }
        if (n > span - pos) {
            n = span - pos;
        }

        c->x = ix + (horiz ? pos : 0);
        c->y = iy + (horiz ? 0 : pos);
        c->w = horiz ? n : iw;
        c->h = horiz ? ih : n;
        pos += n;

        layout(c);
    }
}

/* The layout is only recomputed when the tree changes (layout_dirty), and
 * before anything reads sizes or writes content. */
static void relayout(term_ctx_t *ctx) {
    /* Every scene, not only the active one, so output into a scene that
     * isn't shown still lands in cells of the right size. */
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        term_panel_t *p = &ctx->panels[i];
        if (p->in_use && !p->parent) {
            bool overlay = p->owner != NULL;
            p->x = overlay ? p->req_x : 0;
            p->y = overlay ? p->req_y : 0;
            p->w = overlay ? p->req_w : TERM_COLS;
            p->h = overlay ? p->req_h : TERM_ROWS;
            layout(p);
        }
    }
    ctx->layout_dirty = 0;
    mark_dirty(ctx);
}

static void ensure_layout(term_ctx_t *ctx) {
    if (ctx->layout_dirty) {
        relayout(ctx);
    }
}

int term_panel_width(const term_panel_t *p) {
    ensure_layout(p->ctx);
    return p->iw;
}

int term_panel_height(const term_panel_t *p) {
    ensure_layout(p->ctx);
    return p->ih;
}

/* ---- Composing ----------------------------------------------------------- */

static void put_abs(const term_panel_t *p, int col, int row, uint8_t ch, uint8_t attr) {
    if (col >= 0 && col < TERM_COLS && row >= 0 && row < TERM_ROWS) {
        set_cell(&grid[row][col], p, ch, attr);
    }
}

static void draw_border(const term_panel_t *p, bool focused) {
    uint8_t title_attr = focused ? p->focus_attr : TERM_ATTR_NORMAL;
    if (p->w < 2 || p->h < 2) {
        return;
    }
    int x0 = p->x;
    int y0 = p->y;
    int x1 = p->x + p->w - 1;
    int y1 = p->y + p->h - 1;

    for (int x = x0 + 1; x < x1; x++) {
        put_abs(p, x, y0, TERM_CH_HLINE, TERM_ATTR_NORMAL);
        put_abs(p, x, y1, TERM_CH_HLINE, TERM_ATTR_NORMAL);
    }
    for (int y = y0 + 1; y < y1; y++) {
        put_abs(p, x0, y, TERM_CH_VLINE, TERM_ATTR_NORMAL);
        put_abs(p, x1, y, TERM_CH_VLINE, TERM_ATTR_NORMAL);
    }
    put_abs(p, x0, y0, TERM_CH_TL, TERM_ATTR_NORMAL);
    put_abs(p, x1, y0, TERM_CH_TR, TERM_ATTR_NORMAL);
    put_abs(p, x0, y1, TERM_CH_BL, TERM_ATTR_NORMAL);
    put_abs(p, x1, y1, TERM_CH_BR, TERM_ATTR_NORMAL);

    /* " Title " set into the top edge, in the focus attribute while focused. */
    if (p->title) {
        int x = x0 + 2;
        put_abs(p, x - 1, y0, ' ', title_attr);
        for (const char *s = p->title; *s && x < x1 - 1; s++, x++) {
            put_abs(p, x, y0, (uint8_t)*s, title_attr);
        }
        put_abs(p, x, y0, ' ', title_attr);
    }
}

/* Rebuilds a widget's retained cells from its state. */
static void render_widget(term_panel_t *p) {
    term_cell_t blank;
    set_blank(&blank, p);
    for (int i = 0; i < p->cells_w * p->cells_h; i++) {
        p->cells[i] = blank;
    }
    term_widget_draw(p);
    p->stale = 0;
}

/* Copies the panel tree's retained content into the grid: borders, then
 * each leaf's cells, children over parents. */
static void compose(term_panel_t *p) {
    if (!p->visible || p->w == 0 || p->h == 0) {
        return;
    }
    if (p->stale && p->kind != TERM_KIND_PLAIN) {
        render_widget(p);
    }
    /* A container in a different background than its parent's fills its
     * area, so gaps between its children show its color. */
    if (p->first_child && p->bg != (p->parent ? p->parent->bg : TERM_BG)) {
        term_cell_t blank;
        set_blank(&blank, p);
        for (int r = p->y; r < p->y + p->h; r++) {
            for (int c = p->x; c < p->x + p->w; c++) {
                grid_row[r][c] = blank;
            }
        }
    }
    if (p->border) {
        draw_border(p, p == p->ctx->focus);
    }
    for (int r = 0; r < p->cells_h; r++) {
        memcpy(&grid_row[p->iy + r][p->ix], p->cells + (size_t)r * p->cells_w,
               p->cells_w * sizeof(term_cell_t));
    }
    for (term_panel_t *c = p->first_child; c; c = c->next) {
        compose(c);
    }
}

/* ---- Focus --------------------------------------------------------------- */

/* Widgets draw differently when focused, so both ends of a change redraw. */
static void set_focus(term_ctx_t *ctx, term_panel_t *p) {
    if (ctx->focus != p) {
        if (ctx->focus) {
            term_panel_touch(ctx->focus);
        }
        if (p) {
            term_panel_touch(p);
        }
        ctx->focus = p;
    }
    mark_dirty(ctx);
}

/* A panel can keep focus while it is on screen: visible all the way up the
 * tree, with some area. */
static bool can_focus(const term_panel_t *p) {
    if (!p->w || !p->h) {
        return false;
    }
    const term_panel_t *top = p;
    for (const term_panel_t *a = p; a; a = a->parent) {
        if (!a->visible) {
            return false;
        }
        top = a;
    }
    /* On screen only in the active scene, or one of its overlays. */
    return top == p->ctx->scene || top->owner == p->ctx->scene;
}

void term_panel_set_focusable(term_panel_t *p, bool focusable) {
    p->focusable = focusable;
}

void term_focus(term_ctx_t *ctx, term_panel_t *p) {
    if (!p || p->focusable) {
        set_focus(ctx, p);
    }
}

term_panel_t *term_focused(const term_ctx_t *ctx) {
    return ctx->focus;
}

/* ---- Panel-scoped output ------------------------------------------------- */

void term_put(term_panel_t *p, int col, int row, uint8_t ch, uint8_t attr) {
    if (col < 0 || row < 0 || col >= p->cells_w || row >= p->cells_h) {
        return;
    }
    set_cell(&p->cells[(size_t)row * p->cells_w + col], p, ch, attr);
}

void term_panel_move(term_panel_t *p, int col, int row) {
    p->cur_x = col < 0 ? 0 : (col > 255 ? 255 : col);
    p->cur_y = row < 0 ? 0 : (row > 255 ? 255 : row);
}

void term_panel_set_attr(term_panel_t *p, uint8_t attr) {
    p->attr = attr;
    term_panel_touch(p); /* widgets draw in it */
}

void term_panel_set_colors(term_panel_t *p, uint8_t fg, uint8_t bg) {
    p->fg = fg;
    p->bg = bg;
    term_panel_touch(p);
}

void term_panel_set_focus_attr(term_panel_t *p, uint8_t attr) {
    p->focus_attr = attr;
    term_panel_touch(p);
}

void term_panel_set_align(term_panel_t *p, term_align_t align) {
    p->align = align;
    term_panel_touch(p);
}

void term_panel_set_submit(term_panel_t *p, bool submit) {
    p->submit = submit;
}

void term_panel_set_keys(term_panel_t *p, term_key_fn fn, void *state) {
    p->key_fn = fn;
    p->key_state = state;
}

void term_panel_send(term_panel_t *p, term_event_type_t type, int value) {
    term_emit(p->ctx, type, p, value);
}

void term_panel_wrap(term_panel_t *p, bool wrap) {
    p->wrap = wrap;
}

/* Control characters and escapes for term_panel_putc(); true if `c` was one
 * and is used up. Out of line, so printing plain text only pays one test. */
static __attribute__((noinline)) bool putc_control(term_panel_t *p, char c) {
    if (p->esc) {
        p->esc = 0;
        if (TERM_IS_ESC_ARG(c)) {
            p->attr = c & 0x1F;
            return true;
        }
    }
    switch (c) {
    case TERM_ESC:
        p->esc = 1;
        return true;
    case '\n':
        p->cur_x = 0;
        if (p->cur_y < 255) {
            p->cur_y++;
        }
        return true;
    case '\r':
        p->cur_x = 0;
        return true;
    case '\t': /* moves to the next stop without painting */
        p->cur_x = p->cur_x >= 256 - TERM_TAB ? 255 : (p->cur_x + TERM_TAB) & ~(TERM_TAB - 1);
        return true;
    default:
        return false; /* a CP437 glyph */
    }
}

void term_panel_putc(term_panel_t *p, char c) {
    ensure_layout(p->ctx);
    if (((uint8_t)c < ' ' || p->esc) && putc_control(p, c)) {
        return;
    }
    if (p->wrap && p->cur_x >= p->cells_w) {
        p->cur_x = 0;
        if (p->cur_y < 255) {
            p->cur_y++;
        }
    }
    /* term_put(), inlined: this runs for every character printed. */
    if (p->cur_x < p->cells_w && p->cur_y < p->cells_h) {
        set_cell(&p->cells[(size_t)p->cur_y * p->cells_w + p->cur_x], p, (uint8_t)c, p->attr);
        p->ctx->dirty = 1;
    }
    if (p->cur_x < 255) {
        p->cur_x++;
    }
}

void term_panel_print(term_panel_t *p, const char *str) {
    while (*str) {
        term_panel_putc(p, *str++);
    }
}

/* Our own printf core: CEdev's vsnprintf links nanoprintf, ~7.7 KB. */
void term_vformat(term_out_fn out, void *dst, const char *fmt, va_list args) {
    char num[12]; /* a 32-bit number in decimal, with its sign */
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            out(dst, *fmt);
            continue;
        }
        const char *spec = fmt++;
        bool left = false;
        char pad = ' ';
        for (;; fmt++) {
            if (*fmt == '-') {
                left = true;
            } else if (*fmt == '0') {
                pad = '0';
            } else {
                break;
            }
        }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt++ - '0');
        }
        bool is_long = *fmt == 'l';
        fmt += is_long;

        const char *s = num;
        int len = 1;
        switch (*fmt) {
        case 's':
            s = va_arg(args, const char *);
            if (!s) {
                s = "(null)";
            }
            len = (int)strlen(s);
            pad = ' ';
            break;
        case 'c':
            num[0] = (char)va_arg(args, int);
            break;
        case '%':
            num[0] = '%';
            break;
        case 'd':
        case 'u':
        case 'x':
        case 'X': {
            unsigned long v;
            bool neg = false;
            if (*fmt == 'd') {
                long sv = is_long ? va_arg(args, long) : va_arg(args, int);
                neg = sv < 0;
                v = neg ? 0UL - (unsigned long)sv : (unsigned long)sv;
            } else {
                v = is_long ? va_arg(args, unsigned long) : va_arg(args, unsigned);
            }
            unsigned base = *fmt == 'x' || *fmt == 'X' ? 16 : 10;
            const char *digits = *fmt == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
            char *end = num + sizeof num;
            char *q = end;
            do {
                *--q = digits[v % base];
                v /= base;
            } while (v);
            if (neg && pad == '0') { /* the sign goes before the zeros */
                out(dst, '-');
                width--;
            } else if (neg) {
                *--q = '-';
            }
            s = q;
            len = (int)(end - q);
            break;
        }
        default: /* unsupported: print it as written */
            while (spec < fmt) {
                out(dst, *spec++);
            }
            if (!*fmt) {
                return;
            }
            out(dst, *fmt);
            continue;
        }
        if (!left) {
            for (; width > len; width--) {
                out(dst, pad);
            }
        }
        for (int i = 0; i < len; i++) {
            out(dst, s[i]);
        }
        for (; width > len; width--) {
            out(dst, ' ');
        }
    }
}

static void out_panel(void *dst, char c) {
    term_panel_putc(dst, c);
}

void term_panel_printf(term_panel_t *p, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    term_vformat(out_panel, p, fmt, args);
    va_end(args);
}

void term_panel_repeat(term_panel_t *p, char c, int count) {
    while (count-- > 0) {
        term_panel_putc(p, c);
    }
}

void term_panel_clear(term_panel_t *p) {
    ensure_layout(p->ctx);
    term_cell_t blank;
    set_blank(&blank, p);
    for (int i = 0; i < p->cells_w * p->cells_h; i++) {
        p->cells[i] = blank;
    }
    p->cur_x = 0;
    p->cur_y = 0;
    mark_dirty(p->ctx);
}

/* ---- Input --------------------------------------------------------------- */

typedef struct {
    uint8_t sk;
    char plain; /* what the key types on its own, 0 = nothing */
    char alpha; /* what it types in alpha mode, 0 = nothing */
} keymap_t;

static const keymap_t keymap[] = {
    {sk_0, '0', ' '}, {sk_1, '1', 'Y'}, {sk_2, '2', 'Z'}, {sk_3, '3', 0},
    {sk_4, '4', 'T'}, {sk_5, '5', 'U'}, {sk_6, '6', 'V'}, {sk_7, '7', 'O'},
    {sk_8, '8', 'P'}, {sk_9, '9', 'Q'},
    {sk_DecPnt, '.', ':'}, {sk_Chs, '-', '?'}, {sk_Add, '+', '"'},
    {sk_Sub, '-', 'W'}, {sk_Mul, '*', 'R'}, {sk_Div, '/', 'M'},
    {sk_Comma, ',', 'J'}, {sk_LParen, '(', 'K'}, {sk_RParen, ')', 'L'},
    {sk_Power, '^', 'H'},
    {sk_Math, 0, 'A'}, {sk_Apps, 0, 'B'}, {sk_Prgm, 0, 'C'}, {sk_Recip, 0, 'D'},
    {sk_Sin, 0, 'E'}, {sk_Cos, 0, 'F'}, {sk_Tan, 0, 'G'}, {sk_Square, 0, 'I'},
    {sk_Log, 0, 'N'}, {sk_Ln, 0, 'S'}, {sk_Store, 0, 'X'},
};

int term_alpha_mode(const term_ctx_t *ctx) {
    return ctx->alpha;
}

/* Turns a scan code into an event. Returns false if the key was swallowed
 * (alpha, or a key that types nothing in the current mode). */
static bool translate(term_ctx_t *ctx, uint8_t sk, term_event_t *ev) {
    bool after_2nd = ctx->second;
    ctx->second = 0;

    ev->type = TERM_EV_KEY;
    ev->key = TERM_KEY_NONE;
    ev->ch = 0;
    ev->panel = ctx->focus;
    ev->value = 0;

    if (sk == sk_Alpha) {
        if (after_2nd) {
            ctx->alpha = (ctx->alpha == 2) ? 0 : 2; /* [2nd][alpha]: lock */
        } else {
            ctx->alpha = ctx->alpha ? 0 : 1;
        }
        ev->key = TERM_KEY_ALPHA; /* reported after the change, for status displays */
        return true;
    }

    uint8_t alpha = ctx->alpha;
    if (alpha == 1) {
        ctx->alpha = 0; /* one-shot: applies to this key only */
    }

    switch (sk) {
    case sk_Up:    ev->key = TERM_KEY_UP;    return true;
    case sk_Down:  ev->key = TERM_KEY_DOWN;  return true;
    case sk_Left:  ev->key = TERM_KEY_LEFT;  return true;
    case sk_Right: ev->key = TERM_KEY_RIGHT; return true;
    case sk_Enter: ev->key = TERM_KEY_ENTER; return true;
    case sk_Clear: ev->key = TERM_KEY_CLEAR; return true;
    case sk_Del:   ev->key = TERM_KEY_DEL;   return true;
    case sk_Mode:  ev->key = TERM_KEY_MODE;  return true;
    case sk_Vars:  ev->key = TERM_KEY_VARS;  return true;
    case sk_Yequ:   ev->key = TERM_KEY_F1;   return true;
    case sk_Window: ev->key = TERM_KEY_F2;   return true;
    case sk_Zoom:   ev->key = TERM_KEY_F3;   return true;
    case sk_Trace:  ev->key = TERM_KEY_F4;   return true;
    case sk_Graph:  ev->key = TERM_KEY_F5;   return true;
    case sk_2nd:
        ctx->second = 1;
        ev->key = TERM_KEY_2ND;
        return true;
    default:
        break;
    }

    for (unsigned i = 0; i < sizeof keymap / sizeof keymap[0]; i++) {
        if (keymap[i].sk == sk) {
            char ch = alpha ? keymap[i].alpha : keymap[i].plain;
            if (!ch) {
                return false;
            }
            ev->key = TERM_KEY_CHAR;
            ev->ch = ch;
            return true;
        }
    }
    return false;
}

/* ---- Run loop ------------------------------------------------------------ */

/* The active scene's handler, then the global one, until one returns true. */
static void dispatch(term_ctx_t *ctx, const term_event_t *ev) {
    term_panel_t *scene = ctx->scene;
    if (scene->handler && scene->handler(ctx, ev, scene->handler_state)) {
        return;
    }
    if (ctx->update) {
        ctx->update(ctx, ev, ctx->state);
    }
}

static term_event_t make_event(term_event_type_t type, term_panel_t *panel, int value) {
    term_event_t ev;
    ev.type = type;
    ev.key = TERM_KEY_NONE;
    ev.ch = 0;
    ev.panel = panel;
    ev.value = value;
    return ev;
}

void term_emit(term_ctx_t *ctx, term_event_type_t type, term_panel_t *panel, int value) {
    if (ctx->running) {
        term_event_t ev = make_event(type, panel, value);
        dispatch(ctx, &ev);
    }
}

/* Scene events go only to that scene's own handler. */
static void emit_scene(term_ctx_t *ctx, term_panel_t *scene, term_event_type_t type) {
    if (ctx->running && scene->handler) {
        term_event_t ev = make_event(type, scene, 0);
        scene->handler(ctx, &ev, scene->handler_state);
    }
}

/* A key goes to the focused panel first (its custom key handler, then its
 * widget, then [enter] as a submit if it has that set), then along the
 * handler chain. */
static void dispatch_key(term_ctx_t *ctx, const term_event_t *ev) {
    term_panel_t *p = ctx->focus;
    if (p) {
        if (p->key_fn && p->key_fn(p, ev, p->key_state)) {
            return;
        }
        if (term_widget_key(p, ev)) {
            term_panel_touch(p);
            return;
        }
        if (p->submit && ev->key == TERM_KEY_ENTER) {
            term_emit(ctx, TERM_EV_SUBMIT, p, 0);
            return;
        }
    }
    dispatch(ctx, ev);
}

/* ---- Scenes -------------------------------------------------------------- */

term_panel_t *term_scene_new(term_ctx_t *ctx, term_update_fn handler, void *state) {
    term_panel_t *p = alloc_panel(ctx);
    if (p) {
        p->handler = handler;
        p->handler_state = state;
        ctx->layout_dirty = 1;
    }
    return p;
}

void term_scene_switch(term_ctx_t *ctx, term_panel_t *scene) {
    if (!scene || scene->parent || scene->owner || scene == ctx->scene) {
        return;
    }
    term_panel_t *old = ctx->scene;
    ctx->scene = scene;
    mark_dirty(ctx);
    emit_scene(ctx, old, TERM_EV_SCENE_LEAVE);
    emit_scene(ctx, scene, TERM_EV_SCENE_ENTER);
}

term_panel_t *term_scene_active(const term_ctx_t *ctx) {
    return ctx->scene;
}

/* ---- Overlays ------------------------------------------------------------ */

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

term_panel_t *term_overlay_open(term_ctx_t *ctx, int col, int row, int w, int h) {
    if (ctx->n_overlays == TERM_MAX_OVERLAYS) {
        return NULL;
    }
    term_panel_t *p = alloc_panel(ctx);
    if (!p) {
        return NULL;
    }
    p->owner = ctx->scene;
    p->restore = ctx->focus;
    p->req_x = clamp(col, 0, TERM_COLS);
    p->req_y = clamp(row, 0, TERM_ROWS);
    p->req_w = clamp(w, 0, TERM_COLS - p->req_x);
    p->req_h = clamp(h, 0, TERM_ROWS - p->req_y);
    ctx->overlays[ctx->n_overlays++] = p;
    ctx->layout_dirty = 1;
    return p;
}

term_panel_t *term_overlay_open_centered(term_ctx_t *ctx, int w, int h) {
    w = clamp(w, 0, TERM_COLS);
    h = clamp(h, 0, TERM_ROWS);
    return term_overlay_open(ctx, (TERM_COLS - w) / 2, (TERM_ROWS - h) / 2, w, h);
}

static term_panel_t *root_of(term_panel_t *p) {
    while (p->parent) {
        p = p->parent;
    }
    return p;
}

void term_overlay_close(term_panel_t *ov) {
    if (!ov || !ov->owner) {
        return;
    }
    term_ctx_t *ctx = ov->ctx;
    term_panel_t *back = ov->restore;
    bool give_back = back && (!ctx->focus || root_of(ctx->focus) == ov);

    remove_overlay(ctx, ov);
    free_subtree(ov);
    ctx->layout_dirty = 1;

    if (give_back) {
        ensure_layout(ctx);
        if (can_focus(back)) {
            ctx->focus_lost = 0; /* focus moved back, not lost */
            set_focus(ctx, back);
        }
    }
}

/* Lays out (if the tree changed), composes the retained content and pushes
 * whatever changed to the screen. Does nothing if nothing changed. */
static void frame(term_ctx_t *ctx) {
    ensure_layout(ctx);

    /* titrmlib never picks a new focus itself: if the focused panel went away
     * (hidden, shrunk to nothing, or destroyed), focus becomes empty and the
     * app is told, so it can choose. */
    if (ctx->focus && !can_focus(ctx->focus)) {
        term_panel_t *lost = ctx->focus;
        set_focus(ctx, NULL);
        term_emit(ctx, TERM_EV_FOCUS_LOST, lost, 0);
    }
    if (ctx->focus_lost) {
        ctx->focus_lost = 0;
        term_emit(ctx, TERM_EV_FOCUS_LOST, NULL, 0);
    }
    ensure_layout(ctx); /* the app may have changed the tree in response */

    if (!ctx->dirty) {
        return;
    }
    ctx->dirty = 0;
    for (int r = 0; r < TERM_ROWS; r++) {
        memcpy(grid_row[r], blank_row, sizeof blank_row);
    }
    compose(ctx->scene);
    for (int i = 0; i < ctx->n_overlays; i++) {
        term_panel_t *ov = ctx->overlays[i];
        if (ov->owner == ctx->scene && ov->visible) {
            for (int r = ov->y; r < ov->y + ov->h; r++) { /* opaque */
                memcpy(&grid_row[r][ov->x], blank_row, ov->w * sizeof(term_cell_t));
            }
            compose(ov);
        }
    }
    flush(ctx);
}

term_ctx_t *term_init(void) {
    if (g_open) {
        return &g_ctx;
    }
    memset(&g_ctx, 0, sizeof g_ctx);
    for (int c = 0; c < TERM_COLS; c++) {
        blank_row[c] = blank_cell;
    }
    for (int r = 0; r < TERM_ROWS; r++) {
        grid_row[r] = grid[r];
        memcpy(grid[r], blank_row, sizeof blank_row);
        memcpy(shown[r], blank_row, sizeof blank_row); /* matches the cleared screen */
    }

    g_ctx.root = alloc_panel(&g_ctx);
    g_ctx.scene = g_ctx.root;
    g_ctx.layout_dirty = 1;
    init_row_pixels();

    gfx_Begin();
    gfx_FillScreen(TERM_BG);

    g_open = true;
    init_keys(&g_ctx);
    return &g_ctx;
}

void term_shutdown(term_ctx_t *ctx) {
    if (!g_open) {
        return;
    }
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        if (ctx->panels[i].in_use) {
            term_widget_free(&ctx->panels[i]);
            free_cells(&ctx->panels[i]);
        }
    }
    gfx_End();
    os_ClrHome();
    g_open = false;
}

void term_quit(term_ctx_t *ctx, int result) {
    ctx->quit = 1;
    ctx->result = result;
}

void term_set_tick(term_ctx_t *ctx, unsigned ms) {
    ctx->tick = (unsigned long)ms * CLOCKS_PER_SEC / 1000;
    ctx->last_tick = clock();
}

int term_run(term_ctx_t *ctx, term_update_fn update, void *state) {
    ctx->update = update;
    ctx->state = state;
    ctx->quit = 0;
    ctx->result = 0;
    ctx->running = 1;

    ensure_layout(ctx);
    term_emit(ctx, TERM_EV_START, NULL, 0);
    emit_scene(ctx, ctx->scene, TERM_EV_SCENE_ENTER);
    mark_dirty(ctx);
    frame(ctx);

    while (!ctx->quit) {
        poll_keys(ctx);

        /* Every key waiting in the queue is handled before the next frame. */
        uint8_t sk;
        while (!ctx->quit && (sk = next_key(ctx))) {
            term_event_t ev;
            if (translate(ctx, sk, &ev)) {
                dispatch_key(ctx, &ev);
            }
        }

        if (!ctx->quit && ctx->tick) {
            unsigned long now = clock();
            if (now - ctx->last_tick >= ctx->tick) {
                ctx->last_tick = now;
                term_emit(ctx, TERM_EV_TICK, NULL, 0);
            }
        }

        if (!ctx->quit) {
            frame(ctx);
        }
    }

    ctx->running = 0;
    ctx->update = NULL;
    return ctx->result;
}
