/* Host-side unit tests for titrmlib.
 *
 * The library is built with a native compiler against the stand-in CE
 * headers in tests/stubs/: drawing goes to an in-memory framebuffer and the
 * keypad plays back a scripted list of scan codes, so layout, clipping,
 * focus, widgets and the run loop can all be exercised without a calculator
 * or emulator. titrm.c is included directly so the tests can read the cell
 * grid it composes each frame.
 *
 *     make -C tests
 */

#include "titrm.c"

#include <graphx.h>
#include <keypadc.h>
#include <ti/getcsc.h>

#include <stdio.h>
#include <string.h>

/* ---- Minimal test harness ------------------------------------------------ */

static int failures;
static int checks;
static const char *current;

#define CHECK(cond)                                                              \
    do {                                                                         \
        checks++;                                                                \
        if (!(cond)) {                                                           \
            failures++;                                                          \
            fprintf(stderr, "%s:%d: %s: CHECK(%s) failed\n", __FILE__, __LINE__, \
                    current, #cond);                                             \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                           \
    do {                                                                         \
        long a_ = (long)(a);                                                     \
        long b_ = (long)(b);                                                     \
        checks++;                                                                \
        if (a_ != b_) {                                                          \
            failures++;                                                          \
            fprintf(stderr, "%s:%d: %s: %s == %ld, expected %s == %ld\n",        \
                    __FILE__, __LINE__, current, #a, a_, #b, b_);                \
        }                                                                        \
    } while (0)

#define CHECK_STR(a, b)                                                          \
    do {                                                                         \
        const char *a_ = (a);                                                    \
        const char *b_ = (b);                                                    \
        checks++;                                                                \
        if (strcmp(a_, b_) != 0) {                                               \
            failures++;                                                          \
            fprintf(stderr, "%s:%d: %s: %s == \"%s\", expected \"%s\"\n",        \
                    __FILE__, __LINE__, current, #a, a_, b_);                    \
        }                                                                        \
    } while (0)

/* ---- Fixtures and helpers ------------------------------------------------ */

static term_ctx_t *setup(void) {
    if (g_open) {
        term_shutdown(&g_ctx);
    }
    stub_keys(NULL, 0);
    stub_quit_when_idle = 1;
    return term_init();
}

/* Composes and flushes one frame, as term_run() does after each event. */
static void draw(term_ctx_t *ctx) {
    frame(ctx);
}

static uint8_t ch_at(int col, int row) { return grid[row][col].ch; }
static uint8_t attr_at(int col, int row) {
    return grid[row][col].bg == TERM_FG ? TERM_ATTR_REVERSE : TERM_ATTR_NORMAL;
}

/* `len` cells of a grid row as a string; empty cells read as spaces. */
static const char *text_at(int col, int row, int len) {
    static char buf[TERM_COLS + 1];
    int i;
    for (i = 0; i < len && col + i < TERM_COLS; i++) {
        uint8_t ch = grid[row][col + i].ch;
        buf[i] = ch ? (char)ch : ' ';
    }
    buf[i] = '\0';
    return buf;
}

static bool row_blank(int row, int from, int to) {
    for (int c = from; c < to; c++) {
        if (grid[row][c].ch) {
            return false;
        }
    }
    return true;
}

/* Records every event term_run() hands the app; [clear] quits with 42. */
#define MAX_EVENTS 64
static term_event_t events[MAX_EVENTS];
static int n_events;

static bool record(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (n_events < MAX_EVENTS) {
        events[n_events++] = *ev;
    }
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 42);
    }
    return true;
}

static int run_keys(term_ctx_t *ctx, const uint8_t *keys, int n) {
    n_events = 0;
    stub_keys(keys, n);
    return term_run(ctx, record, NULL);
}

static int count_events(term_event_type_t type) {
    int n = 0;
    for (int i = 0; i < n_events; i++) {
        n += events[i].type == type;
    }
    return n;
}

static const term_event_t *last_event(term_event_type_t type) {
    for (int i = n_events - 1; i >= 0; i--) {
        if (events[i].type == type) {
            return &events[i];
        }
    }
    return NULL;
}

/* ---- Grid and font ------------------------------------------------------- */

static void test_grid_size(void) {
    setup();
    CHECK_EQ(term_cols(), 53);
    CHECK_EQ(term_rows(), 30);
    CHECK(TERM_COLS * TERM_CELL_W <= TERM_SCREEN_W);
    CHECK(TERM_ROWS * TERM_CELL_H <= TERM_SCREEN_H);
}

static bool glyph_empty(uint8_t code) {
    for (int r = 0; r < TERM_GLYPH_H; r++) {
        if (term_font[code].rows[r]) {
            return false;
        }
    }
    return true;
}

static void test_font_table(void) {
    for (int code = 0; code < 256; code++) {
        for (int r = 0; r < TERM_GLYPH_H; r++) {
            CHECK((term_font[code].rows[r] & ~0x1F) == 0); /* 5 pixels per row */
        }
    }
    /* CP437: every code has a glyph but NUL, space and NBSP. */
    for (int code = 0; code < 256; code++) {
        bool blank = code == 0x00 || code == ' ' || code == 0xFF;
        if (glyph_empty((uint8_t)code) != blank) {
            fprintf(stderr, "  0x%02X should %s\n", code, blank ? "be blank" : "have a glyph");
        }
        CHECK(glyph_empty((uint8_t)code) == blank);
    }
    /* Box drawing and blocks join across cells; shades and letters don't. */
    CHECK(is_connected(TERM_CH_VLINE) && is_connected(0xCD) && is_connected(0xDF));
    CHECK(!is_connected(TERM_CH_SHADE) && !is_connected(0xE0) && !is_connected('-'));
}

/* ---- Layout -------------------------------------------------------------- */

static void test_layout_fixed_and_fill(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *root = term_root(ctx);
    term_panel_t *head = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *body = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_panel_t *foot = term_split(root, TERM_VERTICAL, TERM_FIXED(2));
    draw(ctx);

    CHECK_EQ(head->y, 0);
    CHECK_EQ(head->h, 1);
    CHECK_EQ(body->y, 1);
    CHECK_EQ(body->h, 27);
    CHECK_EQ(foot->y, 28);
    CHECK_EQ(foot->h, 2);
    CHECK_EQ(term_panel_width(body), TERM_COLS);
}

static void test_layout_percent(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *left = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_PERCENT(30));
    term_panel_t *right = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FILL);
    draw(ctx);

    CHECK_EQ(left->x, 0);
    CHECK_EQ(left->w, TERM_COLS * 30 / 100);
    CHECK_EQ(right->x, left->w);
    CHECK_EQ(left->w + right->w, TERM_COLS);
    CHECK_EQ(right->h, TERM_ROWS);
}

static void test_layout_fill_weights(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *a = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FILL_WEIGHT(1));
    term_panel_t *b = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FILL_WEIGHT(2));
    draw(ctx);

    CHECK_EQ(a->w, TERM_COLS / 3);          /* 17 */
    CHECK_EQ(b->w, TERM_COLS - TERM_COLS / 3); /* remainder lands on the last */
}

static void test_layout_overflow_clipped(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *a = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(20));
    term_panel_t *b = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(20));
    term_panel_t *c = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    draw(ctx);

    CHECK_EQ(a->h, 20);
    CHECK_EQ(b->h, TERM_ROWS - 20);
    CHECK_EQ(c->h, 0);
}

static void test_layout_border_and_nesting(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *outer = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(outer, true);
    term_panel_t *inner = term_split(outer, TERM_HORIZONTAL, TERM_FIXED(10));
    term_panel_set_border(inner, true);
    draw(ctx);

    CHECK_EQ(term_panel_width(outer), TERM_COLS - 2);
    CHECK_EQ(term_panel_height(outer), TERM_ROWS - 2);
    CHECK_EQ(inner->x, 1);
    CHECK_EQ(inner->y, 1);
    CHECK_EQ(term_panel_width(inner), 8);
    CHECK_EQ(term_panel_height(inner), TERM_ROWS - 4);
}

static void test_layout_hide_show(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *a = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(5));
    term_panel_t *b = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    draw(ctx);
    CHECK_EQ(b->y, 5);

    term_panel_show(a, false);
    CHECK(!term_panel_visible(a));
    draw(ctx);
    CHECK_EQ(b->y, 0);
    CHECK_EQ(b->h, TERM_ROWS);

    term_panel_show(a, true);
    draw(ctx);
    CHECK_EQ(b->y, 5);
    CHECK_EQ(b->h, TERM_ROWS - 5);
}

static void test_split_rules(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *root = term_root(ctx);
    CHECK(term_split(root, TERM_VERTICAL, TERM_FILL) != NULL);
    CHECK(term_split(root, TERM_HORIZONTAL, TERM_FILL) == NULL); /* direction is fixed */
    CHECK(term_split(NULL, TERM_VERTICAL, TERM_FILL) == NULL);

    term_panel_t *list = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_make_list(list, NULL, 0);
    CHECK(term_split(list, TERM_VERTICAL, TERM_FILL) == NULL); /* widgets are leaves */

    /* Root + the two above leaves TERM_MAX_PANELS - 3 free slots. */
    int made = 0;
    while (term_split(root, TERM_VERTICAL, TERM_FIXED(0))) {
        made++;
    }
    CHECK_EQ(made, TERM_MAX_PANELS - 3);
}

static void test_destroy_frees_subtree(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *root = term_root(ctx);
    term_panel_t *a = term_split(root, TERM_VERTICAL, TERM_FIXED(3));
    term_panel_t *b = term_split(root, TERM_VERTICAL, TERM_FIXED(4));
    term_panel_t *c = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_split(b, TERM_HORIZONTAL, TERM_FILL);
    term_make_text(term_split(b, TERM_HORIZONTAL, TERM_FILL), "freed with its panel");

    int used = 0;
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        used += ctx->panels[i].in_use;
    }
    CHECK_EQ(used, 6);

    term_panel_destroy(b);
    used = 0;
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        used += ctx->panels[i].in_use;
    }
    CHECK_EQ(used, 3);
    CHECK(a->next == c);

    draw(ctx);
    CHECK_EQ(c->y, 3);

    term_panel_destroy(c); /* the last child */
    CHECK(root->last_child == a);
    term_panel_destroy(root); /* ignored */
    CHECK(root->in_use);
}

/* ---- Panel-scoped output and clipping ------------------------------------ */

static void print_long_line(term_panel_t *p) {
    term_panel_print(p, "0123456789ABCDEFGHIJ");
    term_panel_move(p, -5, 1);
    term_panel_print(p, "x\ny");
    for (int i = 0; i < 50; i++) {
        term_panel_putc(p, '\n');
    }
    term_panel_print(p, "off the bottom");
}

static void test_print_is_clipped(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *left = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(12));
    term_panel_t *right = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FILL);
    term_panel_set_border(left, true);
    print_long_line(left);
    (void)right;
    draw(ctx);

    CHECK_STR(text_at(1, 1, 10), "0123456789");
    CHECK_EQ(ch_at(11, 1), TERM_CH_VLINE); /* the border survives */
    CHECK(row_blank(1, 12, TERM_COLS));    /* nothing leaks into the sibling */
    CHECK_EQ(ch_at(1, 2), 'x');            /* negative move clamps to 0 */
    CHECK_EQ(ch_at(1, 3), 'y');
    CHECK_EQ(ch_at(0, TERM_ROWS - 1), TERM_CH_BL);
    CHECK(row_blank(TERM_ROWS - 1, 1, 11) == false); /* bottom border intact */
}

static void test_print_wrap_and_attr(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(4));
    term_panel_wrap(p, true);
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_printf(p, "%s-%d", "abcdef", 42);
    draw(ctx);

    CHECK_STR(text_at(0, 0, 4), "abcd");
    CHECK_STR(text_at(0, 1, 4), "ef-4");
    CHECK_STR(text_at(0, 2, 4), "2   ");
    CHECK_EQ(attr_at(0, 0), TERM_ATTR_REVERSE);
    CHECK(row_blank(0, 4, TERM_COLS));
}

static void test_format(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_text(p, "");
    term_text_appendf(p, "%d|%u|%ld|%x|%X|%c|%s|%%", -12345, 54321u, -1234567L, 0xbeefu, 0xBEEFu,
                      'q', "str");
    CHECK_STR(p->u.text.buf, "-12345|54321|-1234567|beef|BEEF|q|str|%");

    term_text_set(p, "");
    term_text_appendf(p, "[%5d][%-5d][%05d][%02X][%-4s][%3s][%s]", 42, 42, -42, 0x7, "ab", "abcd",
                      (char *)NULL);
    CHECK_STR(p->u.text.buf, "[   42][42   ][-0042][07][ab  ][abcd][(null)]");

    /* Unsupported specifiers are printed as written, and output is not cut
     * off at any fixed length. */
    term_text_set(p, "");
    term_text_appendf(p, "%f %.2s %", 1.5);
    CHECK_STR(p->u.text.buf, "%f %.2s %");
    char long_str[201];
    memset(long_str, 'z', 200);
    long_str[200] = '\0';
    term_text_set(p, "");
    term_text_appendf(p, "<%s>", long_str);
    CHECK_EQ(p->u.text.len, 202);
}

static void test_border_and_title(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(5));
    term_panel_set_border(p, true);
    term_panel_set_title(p, "Title");
    draw(ctx);

    CHECK_EQ(ch_at(0, 0), TERM_CH_TL);
    CHECK_EQ(ch_at(TERM_COLS - 1, 0), TERM_CH_TR);
    CHECK_EQ(ch_at(0, 4), TERM_CH_BL);
    CHECK_EQ(ch_at(TERM_COLS - 1, 4), TERM_CH_BR);
    CHECK_EQ(ch_at(1, 0), ' ');
    CHECK_STR(text_at(2, 0, 6), "Title ");
    CHECK_EQ(ch_at(8, 0), TERM_CH_HLINE);
    CHECK_EQ(attr_at(2, 0), TERM_ATTR_NORMAL);

    term_panel_set_focusable(p, true);
    term_focus(ctx, p);
    draw(ctx);
    CHECK_EQ(attr_at(2, 0), TERM_ATTR_REVERSE);
}

/* ---- Pixels -------------------------------------------------------------- */

static void print_glyphs(term_panel_t *p) {
    term_panel_print(p, "A");
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_putc(p, ' ');
}

static void test_glyph_blit(void) {
    term_ctx_t *ctx = setup();
    print_glyphs(term_root(ctx));
    draw(ctx);

    /* 'A' at cell 0,0: glyph bit 4 is the leftmost pixel; column 5 and row 7
     * are the gaps. */
    for (int r = 0; r < TERM_CELL_H; r++) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            bool on = r < TERM_GLYPH_H && x < TERM_GLYPH_W &&
                      (term_font['A'].rows[r] >> (TERM_GLYPH_W - 1 - x)) & 1;
            CHECK_EQ(stub_fb[ORIGIN_Y + r][ORIGIN_X + x], on ? TERM_FG : TERM_BG);
        }
    }
    /* A reversed space is a solid foreground cell. */
    for (int r = 0; r < TERM_CELL_H; r++) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            CHECK_EQ(stub_fb[ORIGIN_Y + r][ORIGIN_X + TERM_CELL_W + x], TERM_FG);
        }
    }
}

static void test_flush_only_redraws_changes(void) {
    term_ctx_t *ctx = setup();
    print_glyphs(term_root(ctx));
    draw(ctx);

    /* Scribble on an unchanged cell: a redraw must leave it alone. */
    int px = ORIGIN_X + 10 * TERM_CELL_W;
    int py = ORIGIN_Y + 10 * TERM_CELL_H;
    stub_fb[py][px] = 0x55;
    draw(ctx);
    CHECK_EQ(stub_fb[py][px], 0x55);

    /* A cell whose content changes is repainted. */
    stub_fb[ORIGIN_Y][ORIGIN_X + 4] = 0x55;
    term_panel_clear(term_root(ctx));
    draw(ctx);
    CHECK_EQ(stub_fb[ORIGIN_Y][ORIGIN_X + 4], TERM_BG);
}

static void test_connected_glyphs_bridge_gaps(void) {
    term_ctx_t *ctx = setup();
    term_panel_set_border(term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(3)), true);
    draw(ctx);

    /* The horizontal line through a HLINE cell runs across the gap column. */
    int row = -1;
    for (int r = 0; r < TERM_GLYPH_H; r++) {
        if (term_font[TERM_CH_HLINE].rows[r] == 0x1F) {
            row = r;
        }
    }
    CHECK(row >= 0);
    if (row >= 0) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            CHECK_EQ(stub_fb[ORIGIN_Y + row][ORIGIN_X + TERM_CELL_W + x], TERM_FG);
        }
    }
}

/* ---- Retained output ----------------------------------------------------- */

static void test_output_is_retained(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(2));
    term_panel_print(p, "kept");
    draw(ctx);
    CHECK_STR(text_at(0, 0, 4), "kept");
    CHECK_EQ(ctx->dirty, 0);

    /* Nothing changed: the next frame composes nothing, so even a scribble on
     * the grid survives it. */
    grid[5][5].ch = 'Z';
    draw(ctx);
    CHECK_EQ(ch_at(5, 5), 'Z');
    CHECK_STR(text_at(0, 0, 4), "kept");

    /* The cursor persists: the next print continues after the last. */
    term_panel_print(p, "!");
    draw(ctx);
    CHECK_STR(text_at(0, 0, 5), "kept!");
    CHECK_EQ(ch_at(5, 5), 0); /* composed from scratch again */

    term_panel_clear(p);
    draw(ctx);
    CHECK(row_blank(0, 0, TERM_COLS));
}

static void test_resize_keeps_content(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *left = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(3));
    term_panel_t *right = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FILL);
    term_panel_print(right, "abcdef\nline 2");
    draw(ctx);
    CHECK_STR(text_at(3, 0, 6), "abcdef");

    term_panel_show(left, false); /* right grows to the full width */
    draw(ctx);
    CHECK_EQ(term_panel_width(right), TERM_COLS);
    CHECK_STR(text_at(0, 0, 6), "abcdef");
    CHECK_STR(text_at(0, 1, 6), "line 2");

    term_panel_show(left, true);
    draw(ctx);
    CHECK_STR(text_at(3, 1, 6), "line 2");
}

static void test_split_panel_has_no_content(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *root = term_root(ctx);
    term_panel_print(root, "before the split");
    term_panel_t *child = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_print(root, "ignored");
    draw(ctx);
    CHECK(root->cells == NULL);
    CHECK(row_blank(0, 0, TERM_COLS)); /* the old content went with the split */
    CHECK(row_blank(5, 0, TERM_COLS));
    CHECK(child->cells != NULL);
}

static void test_keys_are_queued_while_drawing(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *input = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *text = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_input(input);
    term_focus(ctx, input);
    for (int i = 0; i < 20; i++) {
        term_panel_printf(text, "row %d\n", i);
    }

    /* The keypad is read after each redrawn row, so keys pressed during a
     * long frame wait in the queue instead of being lost. */
    static const uint8_t keys[] = {sk_1, sk_2, sk_3};
    stub_keys(keys, 3);
    draw(ctx);
    CHECK_EQ(term_keys_pending(), 3);

    n_events = 0;
    CHECK_EQ(term_run(ctx, record, NULL), STUB_IDLE_RESULT);
    CHECK_STR(term_input_text(input), "123"); /* in order */
    CHECK_EQ(term_keys_pending(), 0);
}

static void test_held_keys_repeat(void) {
    term_ctx_t *ctx = setup();
    stub_quit_when_idle = 0;

    /* An arrow: one press, then repeats after the delay, then at the rate.
     * Time is moved on by hand rather than waited out. */
    stub_hold(sk_Down);
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 1);
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 1); /* not yet */
    ctx->repeat_since -= REPEAT_DELAY;
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 2);
    ctx->repeat_since -= REPEAT_RATE;
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 3);
    stub_release();
    poll_keys(ctx);
    ctx->repeat_since -= REPEAT_DELAY;
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 3); /* released: no more */
    while (next_key(ctx)) {
    }

    /* Other keys don't repeat. */
    stub_hold(sk_1);
    poll_keys(ctx);
    ctx->repeat_since -= REPEAT_DELAY;
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 1);
    stub_release();
    while (next_key(ctx)) {
    }
}

static void test_keys_down_at_start_are_ignored(void) {
    stub_hold(sk_Enter); /* e.g. still held from launching the program */
    term_ctx_t *ctx = setup();
    stub_quit_when_idle = 0;
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 0);

    stub_release();
    poll_keys(ctx);
    stub_hold(sk_Enter); /* pressed again: now it counts */
    poll_keys(ctx);
    CHECK_EQ(term_keys_pending(), 1);
    CHECK_EQ(next_key(ctx), sk_Enter);
    stub_release();
}

/* ---- Focus --------------------------------------------------------------- */

static void test_focus_is_set_by_the_app(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *a = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_t *text = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_t *plain = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_list(a, NULL, 0);
    term_make_text(text, "not focusable");

    /* Nothing is focused until the app says so, even while running. */
    CHECK_EQ(run_keys(ctx, NULL, 0), STUB_IDLE_RESULT);
    CHECK(term_focused(ctx) == NULL);

    term_focus(ctx, a);
    CHECK(term_focused(ctx) == a);
    term_focus(ctx, text); /* not focusable: ignored */
    CHECK(term_focused(ctx) == a);
    term_focus(ctx, plain);
    CHECK(term_focused(ctx) == a);
    term_panel_set_focusable(plain, true);
    term_focus(ctx, plain);
    CHECK(term_focused(ctx) == plain);
    term_focus(ctx, NULL);
    CHECK(term_focused(ctx) == NULL);
}

static void test_focus_lost_is_reported(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *a = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_t *b = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_input(a);
    term_make_input(b);

    /* Hidden: focus empties and the app hears which panel lost it. */
    term_focus(ctx, a);
    term_panel_show(a, false);
    run_keys(ctx, NULL, 0);
    CHECK(term_focused(ctx) == NULL);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 1);
    CHECK(last_event(TERM_EV_FOCUS_LOST)->panel == a);

    /* Destroyed: the handle is gone, so the event carries NULL. */
    term_focus(ctx, b);
    term_panel_destroy(b);
    CHECK(term_focused(ctx) == NULL);
    run_keys(ctx, NULL, 0);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 1);
    CHECK(last_event(TERM_EV_FOCUS_LOST)->panel == NULL);

    /* An explicit "no focus" is not a loss. */
    term_panel_show(a, true);
    term_focus(ctx, a);
    term_focus(ctx, NULL);
    run_keys(ctx, NULL, 0);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 0);
}

/* ---- Run loop and keypad ------------------------------------------------- */

static void test_run_start_and_quit(void) {
    term_ctx_t *ctx = setup();
    static const uint8_t keys[] = {sk_Mode, sk_Clear, sk_Enter};
    CHECK_EQ(run_keys(ctx, keys, 3), 42);

    CHECK_EQ(n_events, 3);
    CHECK_EQ(events[0].type, TERM_EV_START);
    CHECK_EQ(events[1].type, TERM_EV_KEY);
    CHECK_EQ(events[1].key, TERM_KEY_MODE);
    CHECK_EQ(events[2].key, TERM_KEY_CLEAR);
    CHECK(ctx->update == NULL);

    /* With no quit, the stub ends the run when the script runs out. */
    CHECK_EQ(run_keys(ctx, NULL, 0), STUB_IDLE_RESULT);
}

static void test_key_translation(void) {
    term_ctx_t *ctx = setup();
    static const uint8_t keys[] = {
        sk_Up, sk_Down, sk_Left, sk_Right, sk_Del, sk_2nd,
        sk_Yequ, sk_Window, sk_Zoom, sk_Trace, sk_Graph,
        sk_7, sk_DecPnt, sk_Chs, sk_Power,
        sk_Math,           /* types nothing without alpha: swallowed */
        sk_Alpha, sk_Math, /* one-shot alpha -> 'A' */
        sk_Math,           /* alpha has worn off */
        sk_2nd, sk_Alpha,  /* alpha lock */
        sk_Apps, sk_0,     /* 'B', ' ' */
        sk_Alpha,          /* unlock */
        sk_0,
    };
    run_keys(ctx, keys, (int)sizeof keys);

    static const term_key_t want_keys[] = {
        TERM_KEY_UP, TERM_KEY_DOWN, TERM_KEY_LEFT, TERM_KEY_RIGHT, TERM_KEY_DEL,
        TERM_KEY_2ND, TERM_KEY_F1, TERM_KEY_F2, TERM_KEY_F3, TERM_KEY_F4, TERM_KEY_F5,
    };
    int n = (int)(sizeof want_keys / sizeof want_keys[0]);
    for (int i = 0; i < n; i++) {
        CHECK_EQ(events[1 + i].key, want_keys[i]);
    }

    /* \x01 = an [alpha] key event, \x02 = a [2nd] key event */
    const char want_chars[] = "7.-^" "\x01" "A" "\x02\x01" "B " "\x01" "0";
    int e = 1 + n;
    for (int i = 0; want_chars[i]; i++, e++) {
        if (want_chars[i] == '\x01') {
            CHECK_EQ(events[e].key, TERM_KEY_ALPHA);
        } else if (want_chars[i] == '\x02') {
            CHECK_EQ(events[e].key, TERM_KEY_2ND);
        } else {
            CHECK_EQ(events[e].key, TERM_KEY_CHAR);
            CHECK_EQ(events[e].ch, want_chars[i]);
        }
    }
    CHECK_EQ(n_events, e);
    CHECK_EQ(term_alpha_mode(ctx), 0);
}

static void test_alpha_mode_state(void) {
    term_ctx_t *ctx = setup();
    static const uint8_t arm[] = {sk_Alpha};
    run_keys(ctx, arm, 1);
    CHECK_EQ(term_alpha_mode(ctx), 1);
    static const uint8_t lock[] = {sk_2nd, sk_Alpha};
    run_keys(ctx, lock, 2);
    CHECK_EQ(term_alpha_mode(ctx), 2);
    static const uint8_t typed[] = {sk_Math, sk_Math};
    run_keys(ctx, typed, 2);
    CHECK_EQ(term_alpha_mode(ctx), 2); /* lock survives typing */
}

/* The app moves focus itself, here on [vars]. */
static bool focus_on_vars(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    term_panel_t **panels = state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_VARS) {
        term_focus(ctx, term_focused(ctx) == panels[0] ? panels[1] : panels[0]);
    }
    record(ctx, ev, NULL);
    return true;
}

static void test_vars_is_an_ordinary_key(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *panels[2];
    panels[0] = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    panels[1] = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_input(panels[0]);
    term_make_input(panels[1]);
    term_focus(ctx, panels[0]);

    static const uint8_t keys[] = {sk_1, sk_Vars, sk_2, sk_Vars, sk_Vars, sk_3};
    n_events = 0;
    stub_keys(keys, (int)sizeof keys);
    term_run(ctx, focus_on_vars, panels);
    CHECK_STR(term_input_text(panels[0]), "1");
    CHECK_STR(term_input_text(panels[1]), "23");
    CHECK_EQ(count_events(TERM_EV_KEY), 3); /* the three [vars] reach the app */
    CHECK_EQ(last_event(TERM_EV_KEY)->key, TERM_KEY_VARS);
}

static bool quit_after_ticks(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    int *ticks = state;
    if (ev->type == TERM_EV_TICK && ++*ticks == 3) {
        term_quit(ctx, 7);
    }
    return true;
}

static void test_tick(void) {
    term_ctx_t *ctx = setup();
    stub_quit_when_idle = 0; /* only the ticks can end this run */
    term_set_tick(ctx, 1);

    int ticks = 0;
    CHECK_EQ(term_run(ctx, quit_after_ticks, &ticks), 7);
    CHECK_EQ(ticks, 3);

    term_set_tick(ctx, 0);
    CHECK_EQ(ctx->tick, 0);
}

/* ---- Widgets ------------------------------------------------------------- */

static void test_text_wraps_words(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(10));
    term_make_text(p, "the quick brown fox\n  indented supercalifragilistic");
    draw(ctx);

    CHECK_STR(text_at(0, 0, 10), "the quick ");
    CHECK_STR(text_at(0, 1, 10), "brown fox ");
    CHECK_STR(text_at(0, 2, 10), "  indented");
    CHECK_STR(text_at(0, 3, 10), "supercalif"); /* over-long words are split */
    CHECK_STR(text_at(0, 4, 10), "ragilistic");
    CHECK(row_blank(5, 0, 10));
    CHECK(term_focused(ctx) == NULL); /* text is not focusable */

    term_text_set(p, "new");
    draw(ctx);
    CHECK_STR(text_at(0, 0, 10), "new       ");
}

static const char *const items[] = {"zero", "one", "two", "three", "four", "five"};

static void test_list_navigation_and_select(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_list(p, items, 6);
    term_focus(ctx, p);
    CHECK_EQ(term_list_selected(p), 0);

    static const uint8_t keys[] = {sk_Up, sk_Enter, sk_Down, sk_Down, sk_Enter};
    run_keys(ctx, keys, (int)sizeof keys);
    CHECK_EQ(count_events(TERM_EV_SUBMIT), 2);
    CHECK_EQ(count_events(TERM_EV_CHANGE), 3); /* one per move */
    CHECK_EQ(events[1].type, TERM_EV_CHANGE);
    CHECK_EQ(events[1].value, 5); /* up from the top wraps to the bottom */
    CHECK_EQ(events[2].type, TERM_EV_SUBMIT);
    CHECK_EQ(events[2].value, 5);
    CHECK(last_event(TERM_EV_SUBMIT)->panel == p);
    CHECK_EQ(last_event(TERM_EV_SUBMIT)->value, 1);

    draw(ctx);
    CHECK_EQ(ch_at(0, 1), TERM_CH_ARROW_R);
    CHECK_STR(text_at(1, 1, 3), "one");
    CHECK_EQ(attr_at(10, 1), TERM_ATTR_REVERSE); /* focused selection bar */
    CHECK_EQ(attr_at(10, 0), TERM_ATTR_NORMAL);

    term_list_select(p, 99);
    CHECK_EQ(term_list_selected(p), 5);
    term_list_select(p, -1);
    CHECK_EQ(term_list_selected(p), 0);

    term_list_select(p, 5);
    term_list_set_items(p, items, 2);
    CHECK_EQ(term_list_selected(p), 1);
    term_list_set_items(p, items, 0);
    CHECK_EQ(term_list_selected(p), -1);
}

static void test_list_scrolls_with_scrollbar(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(3));
    term_make_list(p, items, 6);
    term_focus(ctx, p);
    term_list_select(p, 4);
    draw(ctx);

    CHECK_STR(text_at(1, 0, 5), "two  ");
    CHECK_STR(text_at(1, 2, 5), "four ");
    CHECK_EQ(ch_at(TERM_COLS - 1, 0), TERM_CH_SHADE);
    CHECK_EQ(ch_at(TERM_COLS - 1, 1), TERM_CH_BLOCK); /* thumb: 4 * 2 / 5 = 1 */
    CHECK_EQ(ch_at(TERM_COLS - 1, 2), TERM_CH_SHADE);
    CHECK(row_blank(3, 0, TERM_COLS));
}

static void test_input_editing(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    term_make_input(p);
    term_focus(ctx, p);

    static const uint8_t keys[] = {
        sk_1, sk_2, sk_Left, sk_3,  /* "132" */
        sk_Right, sk_Right, sk_4,   /* right stops at the end: "1324" */
        sk_Del,                     /* "132" */
        sk_Left, sk_Left, sk_Left, sk_Left, sk_Del, /* at the start: no-op */
        sk_Enter,
    };
    run_keys(ctx, keys, (int)sizeof keys);
    CHECK_STR(term_input_text(p), "132");
    CHECK_EQ(count_events(TERM_EV_CHANGE), 5); /* 4 typed + 1 deleted; moves and no-op [del] don't count */
    CHECK_EQ(count_events(TERM_EV_SUBMIT), 1);
    CHECK(last_event(TERM_EV_SUBMIT)->panel == p);

    draw(ctx);
    CHECK_STR(text_at(0, 0, 4), "132 ");
    CHECK_EQ(attr_at(0, 0), TERM_ATTR_REVERSE); /* cursor at the start */

    /* [clear] empties the field; a second [clear] reaches the app. */
    static const uint8_t clear1[] = {sk_Clear};
    CHECK_EQ(run_keys(ctx, clear1, 1), STUB_IDLE_RESULT);
    CHECK_STR(term_input_text(p), "");
    CHECK_EQ(run_keys(ctx, clear1, 1), 42);

    term_input_set(p, "abc");
    CHECK_STR(term_input_text(p), "abc");
    CHECK_STR(term_input_text(term_root(ctx)), ""); /* not an input */
}

static void test_input_limit_and_scroll(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(5));
    term_make_input(p);
    term_focus(ctx, p);

    char longer[TERM_INPUT_MAX + 11];
    memset(longer, 'x', sizeof longer - 1);
    longer[sizeof longer - 1] = '\0';
    term_input_set(p, longer);
    CHECK_EQ((long)strlen(term_input_text(p)), TERM_INPUT_MAX);

    static const uint8_t more[] = {sk_1};
    run_keys(ctx, more, 1);
    CHECK_EQ((long)strlen(term_input_text(p)), TERM_INPUT_MAX); /* full: ignored */

    term_make_input(p); /* fresh field: see the note on horizontal scroll below */
    static const uint8_t typed[] = {sk_1, sk_2, sk_3, sk_4, sk_5, sk_6, sk_7};
    run_keys(ctx, typed, (int)sizeof typed);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 5), "4567 "); /* scrolled so the cursor (at 7) shows */
    CHECK_EQ(attr_at(4, 0), TERM_ATTR_REVERSE);
    CHECK(row_blank(0, 5, TERM_COLS));

    static const uint8_t home[] = {sk_Left, sk_Left, sk_Left, sk_Left, sk_Left, sk_Left, sk_Left};
    run_keys(ctx, home, (int)sizeof home);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 5), "12345");
    CHECK_EQ(attr_at(0, 0), TERM_ATTR_REVERSE);

    /* Known gap: term_input_set() keeps the old horizontal scroll, so setting
     * a short value after a long one scrolls it out of view. Not asserted
     * until the library resets scroll there. */
}

static void test_text_append_limit_and_scroll(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(3));
    term_make_text(p, "");
    term_text_limit(p, 30); /* "line N\n" is 7 bytes: 4 lines fit */
    term_text_autoscroll(p, true);
    for (int i = 0; i < 7; i++) {
        term_text_appendf(p, "line %d\n", i);
    }
    CHECK_EQ(p->u.text.len, 28);
    CHECK_STR(p->u.text.buf, "line 3\nline 4\nline 5\nline 6\n"); /* oldest dropped */
    draw(ctx);
    CHECK_STR(text_at(0, 0, 6), "line 4"); /* following the end */
    CHECK_STR(text_at(0, 2, 6), "line 6");
    CHECK_EQ(ch_at(TERM_COLS - 1, 0), TERM_CH_ARROW_U); /* more above */

    /* Focused, up/down scroll; leaving the end stops following. */
    term_panel_set_focusable(p, true);
    term_focus(ctx, p);
    static const uint8_t up[] = {sk_Up, sk_Up, sk_Up};
    run_keys(ctx, up, 3);
    CHECK_STR(text_at(0, 0, 6), "line 3"); /* can't scroll past the top */
    CHECK_EQ(ch_at(TERM_COLS - 1, 2), TERM_CH_ARROW_D);
    term_text_append(p, "new\n");
    draw(ctx);
    CHECK(strcmp(text_at(0, 2, 3), "new") != 0); /* not following while scrolled up */

    static const uint8_t down[] = {sk_Down, sk_Down, sk_Down};
    run_keys(ctx, down, 3);
    term_text_append(p, "newest\n");
    draw(ctx);
    CHECK_STR(text_at(0, 2, 6), "newest"); /* back at the end: following again */

    term_text_scroll(p, -100);
    draw(ctx);
    CHECK_EQ(p->u.text.top, 0);

    term_text_clear(p);
    draw(ctx);
    CHECK(row_blank(0, 0, TERM_COLS));
    CHECK_EQ(term_keys_pending(), 0);

    /* Text longer than the limit keeps its end. */
    term_text_append(p, "0123456789012345678901234567890123456789");
    CHECK_EQ(p->u.text.len, 30);
    CHECK_EQ(p->u.text.buf[0], '0' + 40 % 10);
}

static void test_text_align_and_attr(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(9));
    term_make_text(p, "abc\nabcde");
    term_panel_set_align(p, TERM_ALIGN_CENTER);
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 9), "   abc   ");
    CHECK_STR(text_at(0, 1, 9), "  abcde  ");
    CHECK_EQ(attr_at(0, 5), TERM_ATTR_REVERSE); /* the whole panel is filled */
    CHECK_EQ(attr_at(9, 0), TERM_ATTR_NORMAL);
}

static void test_button(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *b = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    term_make_button(b, "OK");
    draw(ctx);
    CHECK_STR(text_at(24, 0, 4), " OK "); /* (53 - 2) / 2 = 25 */
    CHECK_EQ(attr_at(0, 0), TERM_ATTR_REVERSE);
    CHECK_EQ(ch_at(0, 0), 0);

    term_focus(ctx, b);
    draw(ctx);
    CHECK_EQ(ch_at(0, 0), TERM_CH_ARROW_R); /* shows it has focus */

    static const uint8_t keys[] = {sk_Enter};
    run_keys(ctx, keys, 1);
    CHECK_EQ(count_events(TERM_EV_SUBMIT), 1);
    CHECK(last_event(TERM_EV_SUBMIT)->panel == b);
}

static void test_checkbox(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *c = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    term_make_checkbox(c, "Wi-Fi", false);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 9), "[ ] Wi-Fi");
    CHECK(!term_checkbox_checked(c));

    term_focus(ctx, c);
    static const uint8_t keys[] = {sk_Enter};
    run_keys(ctx, keys, 1);
    CHECK(term_checkbox_checked(c));
    CHECK_EQ(count_events(TERM_EV_CHANGE), 1);
    CHECK_EQ(last_event(TERM_EV_CHANGE)->value, 1);
    CHECK_STR(text_at(0, 0, 3), "[x]");
    CHECK_EQ(attr_at(8, 0), TERM_ATTR_REVERSE); /* focused */

    term_checkbox_set(c, false); /* no event */
    CHECK(!term_checkbox_checked(c));
}

/* A custom widget: a counter that right/left change. */
static bool counter_keys(term_panel_t *p, const term_event_t *ev, void *state) {
    int *n = state;
    if (ev->key != TERM_KEY_RIGHT && ev->key != TERM_KEY_LEFT) {
        return false;
    }
    *n += ev->key == TERM_KEY_RIGHT ? 1 : -1;
    term_panel_clear(p);
    term_panel_printf(p, "%d", *n);
    term_panel_send(p, TERM_EV_CHANGE, *n);
    return true;
}

static void test_custom_widget(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    static int count;
    count = 0;
    term_panel_set_keys(p, counter_keys, &count);
    term_panel_set_submit(p, true);
    term_panel_set_focusable(p, true);
    term_focus(ctx, p);

    static const uint8_t keys[] = {sk_Right, sk_Right, sk_Left, sk_Right, sk_Up, sk_Enter};
    run_keys(ctx, keys, (int)sizeof keys);
    CHECK_EQ(count, 2);
    CHECK_STR(text_at(0, 0, 2), "2 ");
    CHECK_EQ(count_events(TERM_EV_CHANGE), 4);
    CHECK_EQ(last_event(TERM_EV_CHANGE)->value, 2);
    CHECK(last_event(TERM_EV_CHANGE)->panel == p);
    CHECK_EQ(count_events(TERM_EV_KEY), 1); /* up: not used, so the app gets it */
    CHECK_EQ(count_events(TERM_EV_SUBMIT), 1); /* [enter], from set_submit */
}

static void test_colors(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *box = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(6));
    term_panel_set_colors(box, TERM_COLOR_YELLOW, TERM_COLOR_BLUE);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "Box");
    term_panel_t *a = term_split(box, TERM_VERTICAL, TERM_FIXED(1)); /* inherits */
    term_panel_t *b = term_split(box, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_set_colors(b, TERM_COLOR_RED, TERM_COLOR_WHITE);
    term_panel_print(a, "A");
    term_panel_set_attr(a, TERM_ATTR_REVERSE);
    term_panel_print(a, "B");
    term_panel_print(b, "C");
    draw(ctx);

    term_cell_t c = grid[1][1]; /* "A" */
    CHECK_EQ(c.ch, 'A');
    CHECK_EQ(c.fg, TERM_COLOR_YELLOW);
    CHECK_EQ(c.bg, TERM_COLOR_BLUE);
    CHECK_EQ(grid[1][2].fg, TERM_COLOR_BLUE); /* reversed: swapped */
    CHECK_EQ(grid[1][2].bg, TERM_COLOR_YELLOW);
    CHECK_EQ(grid[1][3].bg, TERM_COLOR_BLUE); /* a blank cell of "a" */
    CHECK_EQ(grid[2][1].fg, TERM_COLOR_RED);  /* "C" */
    CHECK_EQ(grid[2][1].bg, TERM_COLOR_WHITE);
    CHECK_EQ(grid[0][0].fg, TERM_COLOR_YELLOW); /* border */
    CHECK_EQ(grid[0][0].bg, TERM_COLOR_BLUE);
    CHECK_EQ(grid[0][2].bg, TERM_COLOR_BLUE); /* title, not focused: normal */
    CHECK_EQ(grid[4][5].bg, TERM_COLOR_BLUE);   /* the box's own area, under its children */
    CHECK_EQ(grid[6][0].bg, TERM_COLOR_BLACK);  /* outside it */

    /* Pixels: foreground where the glyph is set, background elsewhere. */
    for (int r = 0; r < TERM_CELL_H; r++) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            bool on = r < TERM_GLYPH_H && x < TERM_GLYPH_W &&
                      (term_font['A'].rows[r] >> (TERM_GLYPH_W - 1 - x)) & 1;
            CHECK_EQ(stub_fb[ORIGIN_Y + TERM_CELL_H + r][ORIGIN_X + TERM_CELL_W + x],
                     on ? TERM_COLOR_YELLOW : TERM_COLOR_BLUE);
        }
    }

    /* Clearing fills with the background; widgets draw in the colors. */
    term_panel_set_colors(a, TERM_COLOR_WHITE, TERM_COLOR_GREEN);
    term_panel_clear(a);
    term_make_button(b, "OK");
    draw(ctx);
    CHECK_EQ(grid[1][1].bg, TERM_COLOR_GREEN);
    CHECK_EQ(grid[2][1].bg, TERM_COLOR_RED); /* a button is reversed */
    CHECK_EQ(grid[2][1].fg, TERM_COLOR_WHITE);
}

static void test_focus_attr(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(5));
    term_panel_set_border(p, true);
    term_panel_set_title(p, "List");
    term_make_list(p, items, 3);
    term_focus(ctx, p);
    draw(ctx);
    CHECK_EQ(attr_at(2, 0), TERM_ATTR_REVERSE); /* title */
    CHECK_EQ(attr_at(5, 1), TERM_ATTR_REVERSE); /* selected row */

    term_panel_set_focus_attr(p, TERM_ATTR_NORMAL); /* no highlight */
    draw(ctx);
    CHECK_EQ(attr_at(2, 0), TERM_ATTR_NORMAL);
    CHECK_EQ(attr_at(5, 1), TERM_ATTR_NORMAL);
    CHECK_EQ(ch_at(1, 1), TERM_CH_ARROW_R); /* the arrow still marks the selection */
}

static void test_progress(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *p = term_split(term_root(ctx), TERM_HORIZONTAL, TERM_FIXED(10));
    term_make_progress(p, 4);

    term_progress_set(p, 1);
    draw(ctx);
    CHECK_EQ(ch_at(0, 0), TERM_CH_BLOCK);
    CHECK_EQ(ch_at(1, 0), TERM_CH_BLOCK);
    CHECK_EQ(ch_at(2, 0), TERM_CH_SHADE); /* 1/4 of 10 = 2 cells */

    term_progress_set(p, 99);
    draw(ctx);
    CHECK_EQ(ch_at(9, 0), TERM_CH_BLOCK);
    term_progress_set(p, -1);
    draw(ctx);
    CHECK_EQ(ch_at(0, 0), TERM_CH_SHADE);
}

/* ---- Scenes -------------------------------------------------------------- */

/* A scene handler that logs what it sees and consumes the '1' key. */
typedef struct {
    term_event_t seen[16];
    int n;
} scene_log_t;

static bool scene_handler(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)ctx;
    scene_log_t *log = state;
    if (log->n < 16) {
        log->seen[log->n++] = *ev;
    }
    return ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CHAR && ev->ch == '1';
}

static void test_scenes_keep_their_content(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *first = term_root(ctx);
    term_panel_t *a = term_split(first, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_print(a, "first");

    term_panel_t *second = term_scene_new(ctx, NULL, NULL);
    CHECK(second != NULL);
    term_panel_t *b = term_split(second, TERM_VERTICAL, TERM_FIXED(2));
    term_panel_print(b, "second");
    CHECK(term_scene_active(ctx) == first);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 6), "first ");

    term_scene_switch(ctx, second);
    CHECK(term_scene_active(ctx) == second);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 6), "second");

    term_panel_print(a, "!"); /* printing into a scene that isn't shown */
    term_scene_switch(ctx, first);
    draw(ctx);
    CHECK_STR(text_at(0, 0, 6), "first!");

    term_scene_switch(ctx, b); /* not a scene: ignored */
    CHECK(term_scene_active(ctx) == first);
}

static void test_scene_event_chain(void) {
    term_ctx_t *ctx = setup();
    static scene_log_t mine, other;
    memset(&mine, 0, sizeof mine);
    memset(&other, 0, sizeof other);
    term_panel_t *scene = term_scene_new(ctx, scene_handler, &mine);
    term_scene_new(ctx, scene_handler, &other); /* never active */
    term_scene_switch(ctx, scene);

    static const uint8_t keys[] = {sk_1, sk_2};
    run_keys(ctx, keys, 2);

    /* The scene hears START, its own ENTER, then both keys, and consumes '1'. */
    CHECK_EQ(mine.n, 4);
    CHECK_EQ(mine.seen[0].type, TERM_EV_START);
    CHECK_EQ(mine.seen[1].type, TERM_EV_SCENE_ENTER);
    CHECK(mine.seen[1].panel == scene);
    CHECK_EQ(mine.seen[2].ch, '1');
    CHECK_EQ(mine.seen[3].ch, '2');

    /* The global handler gets START and only the key the scene let through;
     * scene events never reach it. */
    CHECK_EQ(n_events, 2);
    CHECK_EQ(events[0].type, TERM_EV_START);
    CHECK_EQ(events[1].ch, '2');
    CHECK_EQ(count_events(TERM_EV_SCENE_ENTER), 0);

    CHECK_EQ(other.n, 0); /* inactive scenes get nothing */
}

/* Global handler that switches scenes on [vars]. */
static bool switch_on_vars(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    term_panel_t *to = state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_VARS) {
        term_scene_switch(ctx, to);
    }
    return record(ctx, ev, NULL);
}

static void test_scene_enter_and_leave(void) {
    term_ctx_t *ctx = setup();
    static scene_log_t first_log, second_log;
    memset(&first_log, 0, sizeof first_log);
    memset(&second_log, 0, sizeof second_log);
    term_panel_t *first = term_scene_new(ctx, scene_handler, &first_log);
    term_panel_t *second = term_scene_new(ctx, scene_handler, &second_log);
    term_scene_switch(ctx, first); /* before term_run: no events yet */
    CHECK_EQ(first_log.n, 0);

    static const uint8_t keys[] = {sk_Vars};
    n_events = 0;
    stub_keys(keys, 1);
    term_run(ctx, switch_on_vars, second);

    CHECK(term_scene_active(ctx) == second);
    CHECK_EQ(first_log.seen[first_log.n - 1].type, TERM_EV_SCENE_LEAVE);
    CHECK(first_log.seen[first_log.n - 1].panel == first);
    CHECK_EQ(second_log.n, 1);
    CHECK_EQ(second_log.seen[0].type, TERM_EV_SCENE_ENTER);
    CHECK(second_log.seen[0].panel == second);
}

static void test_scene_switch_loses_focus(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *input = term_split(term_root(ctx), TERM_VERTICAL, TERM_FIXED(1));
    term_make_input(input);
    term_focus(ctx, input);
    term_panel_t *other = term_scene_new(ctx, NULL, NULL);

    term_scene_switch(ctx, other);
    run_keys(ctx, NULL, 0);
    CHECK(term_focused(ctx) == NULL);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 1);
    CHECK(last_event(TERM_EV_FOCUS_LOST)->panel == input);

    /* A panel in a scene that isn't active can't take focus while it's hidden
     * away; once its scene is back, it can. */
    term_scene_switch(ctx, term_root(ctx));
    term_focus(ctx, input);
    run_keys(ctx, NULL, 0);
    CHECK(term_focused(ctx) == input);
}

static void test_scene_destroy(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *scene = term_scene_new(ctx, NULL, NULL);
    term_split(scene, TERM_VERTICAL, TERM_FILL);
    int used = 0;
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        used += ctx->panels[i].in_use;
    }
    CHECK_EQ(used, 3);

    term_scene_switch(ctx, scene);
    term_panel_destroy(scene); /* active: ignored */
    CHECK(scene->in_use);
    term_panel_destroy(term_root(ctx)); /* the first scene: ignored */
    CHECK(term_root(ctx)->in_use);

    term_scene_switch(ctx, term_root(ctx));
    term_panel_destroy(scene);
    used = 0;
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        used += ctx->panels[i].in_use;
    }
    CHECK_EQ(used, 1);
}

/* ---- Overlays ------------------------------------------------------------ */

static void test_overlay_draws_on_top(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *under = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    for (int i = 0; i < TERM_ROWS; i++) {
        term_panel_move(under, 0, i);
        term_panel_repeat(under, '.', TERM_COLS);
    }
    term_panel_t *ov = term_overlay_open(ctx, 10, 5, 8, 3);
    CHECK(ov != NULL);
    term_panel_set_border(ov, true);
    term_panel_print(ov, "hi");
    draw(ctx);

    CHECK_EQ(ch_at(10, 5), TERM_CH_TL);
    CHECK_STR(text_at(11, 6, 6), "hi    "); /* opaque: no dots show through */
    CHECK_EQ(ch_at(17, 7), TERM_CH_BR);
    CHECK_EQ(ch_at(9, 6), '.');
    CHECK_EQ(ch_at(18, 6), '.');

    term_overlay_close(ov);
    draw(ctx);
    CHECK_STR(text_at(10, 6, 8), "........"); /* the retained content is back */
}

static void test_overlay_position(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *ov = term_overlay_open_centered(ctx, 21, 10);
    CHECK_EQ(term_panel_width(ov), 21);
    CHECK_EQ(ov->x, (TERM_COLS - 21) / 2);
    CHECK_EQ(ov->y, (TERM_ROWS - 10) / 2);

    term_panel_t *edge = term_overlay_open(ctx, 50, 28, 10, 10); /* clipped */
    CHECK_EQ(term_panel_width(edge), TERM_COLS - 50);
    CHECK_EQ(term_panel_height(edge), TERM_ROWS - 28);
}

static void test_overlay_gives_focus_back(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *list = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_t *other = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_list(list, items, 6);
    term_make_input(other);
    term_focus(ctx, list);

    /* Opening doesn't move focus; the app moves it in and types there. */
    term_panel_t *ov = term_overlay_open_centered(ctx, 20, 3);
    term_panel_t *field = term_split(ov, TERM_VERTICAL, TERM_FILL);
    term_make_input(field);
    CHECK(term_focused(ctx) == list);
    term_focus(ctx, field);
    static const uint8_t keys[] = {sk_1};
    run_keys(ctx, keys, 1);
    CHECK_STR(term_input_text(field), "1");

    /* Closing while focus is inside: focus goes back to the list. */
    term_overlay_close(ov);
    CHECK(term_focused(ctx) == list);
    run_keys(ctx, NULL, 0);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 0);

    /* The app moved focus elsewhere before closing: left alone. */
    ov = term_overlay_open_centered(ctx, 20, 3);
    term_focus(ctx, other);
    term_overlay_close(ov);
    CHECK(term_focused(ctx) == other);

    /* Focus was empty: the remembered panel gets it back. */
    ov = term_overlay_open_centered(ctx, 20, 3);
    term_focus(ctx, NULL);
    term_overlay_close(ov);
    CHECK(term_focused(ctx) == other);

    /* The remembered panel went away: nothing to give back, focus is lost. */
    ov = term_overlay_open_centered(ctx, 20, 3);
    field = term_split(ov, TERM_VERTICAL, TERM_FILL);
    term_make_input(field);
    term_focus(ctx, field);
    term_panel_destroy(other);
    term_overlay_close(ov);
    CHECK(term_focused(ctx) == NULL);
    run_keys(ctx, NULL, 0);
    CHECK_EQ(count_events(TERM_EV_FOCUS_LOST), 1);
}

static void test_overlays_stack(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *list = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_make_list(list, items, 6);
    term_focus(ctx, list);

    term_panel_t *first = term_overlay_open(ctx, 0, 0, 10, 3);
    term_panel_t *a = term_split(first, TERM_VERTICAL, TERM_FILL);
    term_make_input(a);
    term_focus(ctx, a);
    term_panel_t *second = term_overlay_open(ctx, 5, 1, 10, 3);
    term_panel_t *b = term_split(second, TERM_VERTICAL, TERM_FILL);
    term_make_input(b);
    term_focus(ctx, b);
    term_input_set(a, "AAAAAAAAAA"); /* inputs draw on their first row */
    term_input_set(b, "BBBBBBBBBB");
    draw(ctx);
    CHECK_EQ(ch_at(5, 0), 'A');
    CHECK_EQ(ch_at(5, 1), 'B'); /* the newer overlay is on top of the older */
    CHECK_EQ(ch_at(4, 1), 0);   /* and the older one is opaque */

    term_overlay_close(second);
    CHECK(term_focused(ctx) == a);
    term_overlay_close(first);
    CHECK(term_focused(ctx) == list);

    for (int i = 0; i < TERM_MAX_OVERLAYS; i++) {
        CHECK(term_overlay_open(ctx, 0, 0, 1, 1) != NULL);
    }
    CHECK(term_overlay_open(ctx, 0, 0, 1, 1) == NULL); /* the limit */
}

static void test_overlays_belong_to_a_scene(void) {
    term_ctx_t *ctx = setup();
    term_panel_t *ov = term_overlay_open(ctx, 0, 0, 5, 1);
    term_panel_print(ov, "OVER");
    term_panel_t *input = term_split(ov, TERM_VERTICAL, TERM_FILL);
    term_panel_destroy(input); /* splitting then removing: ov is a leaf again */
    term_panel_print(ov, "OVER");
    draw(ctx);
    CHECK_STR(text_at(0, 0, 4), "OVER");

    term_panel_t *other = term_scene_new(ctx, NULL, NULL);
    term_scene_switch(ctx, other);
    draw(ctx);
    CHECK(row_blank(0, 0, TERM_COLS)); /* not shown over another scene */
    CHECK(term_scene_active(ctx) == other);
    term_scene_switch(ctx, ov); /* an overlay is not a scene */
    CHECK(term_scene_active(ctx) == other);

    term_scene_switch(ctx, term_root(ctx));
    draw(ctx);
    CHECK_STR(text_at(0, 0, 4), "OVER");

    /* Removing a scene closes its overlays. */
    term_scene_switch(ctx, other);
    term_panel_t *mine = term_overlay_open(ctx, 0, 0, 3, 3);
    term_split(mine, TERM_VERTICAL, TERM_FILL);
    term_scene_switch(ctx, term_root(ctx));
    term_panel_destroy(other);
    CHECK_EQ(ctx->n_overlays, 1);
    int used = 0;
    for (int i = 0; i < TERM_MAX_PANELS; i++) {
        used += ctx->panels[i].in_use;
    }
    CHECK_EQ(used, 2); /* root and the first overlay */
}

/* ---- Lifecycle ----------------------------------------------------------- */

static void test_init_and_shutdown(void) {
    term_ctx_t *ctx = setup();
    CHECK(stub_gfx_open);
    CHECK(term_init() == ctx); /* a second init returns the live context */
    CHECK(term_root(ctx) != NULL);
    CHECK_EQ(term_root(ctx)->w, 0); /* laid out lazily */
    term_shutdown(ctx);
    CHECK(!stub_gfx_open);
    term_shutdown(ctx); /* harmless twice */
}

/* ---- Runner -------------------------------------------------------------- */

#define TEST(fn) {#fn, fn}

static const struct {
    const char *name;
    void (*fn)(void);
} tests[] = {
    TEST(test_grid_size),
    TEST(test_font_table),
    TEST(test_layout_fixed_and_fill),
    TEST(test_layout_percent),
    TEST(test_layout_fill_weights),
    TEST(test_layout_overflow_clipped),
    TEST(test_layout_border_and_nesting),
    TEST(test_layout_hide_show),
    TEST(test_split_rules),
    TEST(test_destroy_frees_subtree),
    TEST(test_print_is_clipped),
    TEST(test_print_wrap_and_attr),
    TEST(test_format),
    TEST(test_border_and_title),
    TEST(test_glyph_blit),
    TEST(test_flush_only_redraws_changes),
    TEST(test_connected_glyphs_bridge_gaps),
    TEST(test_output_is_retained),
    TEST(test_resize_keeps_content),
    TEST(test_split_panel_has_no_content),
    TEST(test_keys_are_queued_while_drawing),
    TEST(test_held_keys_repeat),
    TEST(test_keys_down_at_start_are_ignored),
    TEST(test_focus_is_set_by_the_app),
    TEST(test_focus_lost_is_reported),
    TEST(test_run_start_and_quit),
    TEST(test_key_translation),
    TEST(test_alpha_mode_state),
    TEST(test_vars_is_an_ordinary_key),
    TEST(test_tick),
    TEST(test_text_wraps_words),
    TEST(test_list_navigation_and_select),
    TEST(test_list_scrolls_with_scrollbar),
    TEST(test_input_editing),
    TEST(test_input_limit_and_scroll),
    TEST(test_text_append_limit_and_scroll),
    TEST(test_text_align_and_attr),
    TEST(test_button),
    TEST(test_checkbox),
    TEST(test_custom_widget),
    TEST(test_focus_attr),
    TEST(test_colors),
    TEST(test_progress),
    TEST(test_scenes_keep_their_content),
    TEST(test_scene_event_chain),
    TEST(test_scene_enter_and_leave),
    TEST(test_scene_switch_loses_focus),
    TEST(test_scene_destroy),
    TEST(test_overlay_draws_on_top),
    TEST(test_overlay_position),
    TEST(test_overlay_gives_focus_back),
    TEST(test_overlays_stack),
    TEST(test_overlays_belong_to_a_scene),
    TEST(test_init_and_shutdown),
};

int main(void) {
    int n = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < n; i++) {
        int before = failures;
        current = tests[i].name;
        tests[i].fn();
        printf("%s %s\n", failures == before ? "ok  " : "FAIL", current);
    }
    if (g_open) {
        term_shutdown(&g_ctx);
    }
    printf("\n%d tests, %d checks, %d failed\n", n, checks, failures);
    return failures ? 1 : 0;
}
