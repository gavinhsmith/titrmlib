/*
 * Hardware test: selfcheck. Runs assertions on the calculator itself, where
 * the compiler (24-bit int, ez80 libc) and graphx are the real ones, then
 * replaces the screen with a checklist. The expected screen is the one where
 * every line has a check mark.
 *
 * Screen checks read pixels back from VRAM after the first frame and compare
 * them with the font table, so they test what was actually drawn.
 *
 * Fixture (rows):  0-3 "fixture" text   4-8 clip | blank | reverse
 *                  9-28 log             29 progress bar
 */

#include <graphx.h>
#include <stdio.h>
#include <string.h>

#include "titrm.h"
#include "titrm_font.h"

#define MAX_CHECKS 24

typedef struct {
    const char *name;
    bool ok;
} check_t;

static check_t checks[MAX_CHECKS];
static int num_checks;

static void check(const char *name, bool ok) {
    if (num_checks < MAX_CHECKS) {
        checks[num_checks].name = name;
        checks[num_checks].ok = ok;
        num_checks++;
    }
}

/* ---- Pixel readback ------------------------------------------------------ */

/* Mirrors titrm.c: the grid is centred, cells are TERM_CELL_W x TERM_CELL_H,
 * light-on-dark with palette entries 0xFF / 0x00. Not valid for box-drawing
 * characters, which are stretched into the gap. */
#define ORIGIN_X ((TERM_SCREEN_W - TERM_COLS * TERM_CELL_W) / 2)
#define ORIGIN_Y ((TERM_SCREEN_H - TERM_ROWS * TERM_CELL_H) / 2)

static bool cell_is(int col, int row, uint8_t ch, bool reverse) {
    uint8_t fg = reverse ? 0x00 : 0xFF;
    uint8_t bg = reverse ? 0xFF : 0x00;
    int px = ORIGIN_X + col * TERM_CELL_W;
    int py = ORIGIN_Y + row * TERM_CELL_H;
    for (int y = 0; y < TERM_CELL_H; y++) {
        for (int x = 0; x < TERM_CELL_W; x++) {
            bool on = ch != 0 && ch != ' ' && y < TERM_GLYPH_H && x < TERM_GLYPH_W &&
                      (term_font[ch].rows[y] >> (TERM_GLYPH_W - 1 - x) & 1);
            if (gfx_GetPixel(px + x, py + y) != (on ? fg : bg)) {
                return false;
            }
        }
    }
    return true;
}

static bool text_at(int col, int row, const char *s, bool reverse) {
    for (; *s; s++, col++) {
        if (!cell_is(col, row, (uint8_t)*s, reverse)) {
            return false;
        }
    }
    return true;
}

static bool blank_rect(int col, int row, int w, int h) {
    for (int r = row; r < row + h; r++) {
        for (int c = col; c < col + w; c++) {
            if (!cell_is(c, r, ' ', false)) {
                return false;
            }
        }
    }
    return true;
}

/* ---- Fixture ------------------------------------------------------------- */

typedef struct {
    term_panel_t *text;
    term_panel_t *row;
    term_panel_t *clip;
    term_panel_t *log;
    term_panel_t *progress;
    term_panel_t *input;
    term_panel_t *list;
    bool done;
} app_t;

static const char *const items[] = {"one", "two", "three"};

static void print_text(term_panel_t *p) {
    term_panel_print(p, "Hello, CE! \xE1\n");
    term_panel_printf(p, "%d|%u|%ld|%x", -12345, 54321u, 1234567L, 0xBEEFu);
}

static void print_clip(term_panel_t *p) {
    term_panel_print(p, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    term_panel_move(p, 0, 5);
    term_panel_print(p, "XXXXXXXXXX");
}

static void print_reverse(term_panel_t *p) {
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_print(p, "REV");
}

static void print_results(term_panel_t *p) {
    int failed = 0;
    for (int i = 0; i < num_checks; i++) {
        term_panel_print(p, checks[i].ok ? "+ " : "x ");
        term_panel_print(p, checks[i].name);
        term_panel_putc(p, '\n');
        failed += !checks[i].ok;
    }
    term_panel_putc(p, '\n');
    if (failed) {
        term_panel_set_attr(p, TERM_ATTR_REVERSE);
        term_panel_printf(p, " %d of %d FAILED ", failed, num_checks);
    } else {
        term_panel_printf(p, "all %d passed", num_checks);
    }
}

/* ---- Checks -------------------------------------------------------------- */

static void run_checks(term_ctx_t *ctx, app_t *app) {
    char buf[40];
    term_panel_t *root = term_root(ctx);

    check("grid is 53x30", term_cols() == 53 && term_rows() == 30);
    check("layout: sizes and borders",
          term_panel_width(app->text) == 51 && term_panel_height(app->text) == 2 &&
              term_panel_width(app->clip) == 8 && term_panel_height(app->clip) == 3 &&
              term_panel_width(app->log) == 53 && term_panel_height(app->log) == 20 &&
              term_panel_width(app->progress) == 53);

    snprintf(buf, sizeof buf, "%d|%u|%ld|%x", -12345, 54321u, 1234567L, 0xBEEFu);
    check("libc: snprintf with 24-bit int", strcmp(buf, "-12345|54321|1234567|beef") == 0);

    check("screen: plain text", text_at(1, 1, "Hello, CE!", false));
    check("screen: glyph 0xE1 from a string", cell_is(12, 1, 0xE1, false));
    check("screen: panel printf", text_at(1, 2, "-12345|54321|1234567|beef", false));
    check("screen: text clipped to panel", text_at(1, 5, "XXXXXXXX", false));
    check("screen: nothing leaks to sibling", blank_rect(10, 4, 10, 5));
    check("screen: reverse video", text_at(20, 4, "REV", true) && cell_is(23, 4, ' ', false));
    check("screen: text follows its end",
          text_at(0, 28, "line 250", false) && text_at(0, 9, "line 231", false));
    check("screen: progress 999/1000",
          cell_is(52, 29, TERM_CH_SHADE, false) && !cell_is(51, 29, TERM_CH_SHADE, false));

    check("focus: none until the app sets it", term_focused(ctx) == NULL);
    term_focus(ctx, app->text); /* a plain panel, not focusable */
    check("focus: refuses a plain panel", term_focused(ctx) == NULL);

    char long_text[61];
    memset(long_text, 'a', 60);
    long_text[60] = '\0';
    term_input_set(app->input, long_text);
    check("input: truncates to 48", strlen(term_input_text(app->input)) == TERM_INPUT_MAX);

    term_list_select(app->list, 1000);
    bool high = term_list_selected(app->list) == 2;
    term_list_select(app->list, -5);
    check("list: select clamps", high && term_list_selected(app->list) == 0);

    check("split: direction mismatch", term_split(root, TERM_HORIZONTAL, TERM_FILL) == NULL);

    /* 10 panels are in use (root + 9); the rest of the pool must be usable. */
    term_panel_t *extra = term_split(root, TERM_VERTICAL, TERM_FIXED(0));
    int got = extra ? 1 : 0;
    while (extra && term_split(extra, TERM_VERTICAL, TERM_FIXED(0))) {
        got++;
    }
    check("split: pool of 32 panels", got == TERM_MAX_PANELS - 10);
    if (extra) {
        term_panel_destroy(extra);
    }
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    switch (ev->type) {
    case TERM_EV_START:
        for (int i = 1; i <= 250; i++) {
            term_text_appendf(app->log, "line %d\n", i);
        }
        term_set_tick(ctx, 10); /* checks run after the first frame is drawn */
        break;
    case TERM_EV_TICK:
        if (app->done) {
            break;
        }
        app->done = true;
        term_set_tick(ctx, 0);
        run_checks(ctx, app);

        /* Swap the fixture for the results. */
        term_panel_t *fixture[] = {app->text, app->row, app->log, app->progress, app->input, app->list};
        for (unsigned i = 0; i < sizeof fixture / sizeof fixture[0]; i++) {
            term_panel_destroy(fixture[i]);
        }
        term_panel_t *results = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
        term_panel_set_border(results, true);
        term_panel_set_title(results, "selfcheck");
        print_results(results);
        break;
    case TERM_EV_KEY:
        if (ev->key == TERM_KEY_CLEAR) {
            term_quit(ctx, 0);
        }
        break;
    default:
        break;
    }
    return true;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    app.text = term_split(root, TERM_VERTICAL, TERM_FIXED(4));
    term_panel_set_border(app.text, true);
    term_panel_set_title(app.text, "fixture");
    print_text(app.text);

    app.row = term_split(root, TERM_VERTICAL, TERM_FIXED(5));
    app.clip = term_split(app.row, TERM_HORIZONTAL, TERM_FIXED(10));
    term_panel_set_border(app.clip, true);
    print_clip(app.clip);
    term_split(app.row, TERM_HORIZONTAL, TERM_FIXED(10)); /* must stay blank */
    term_panel_t *rev = term_split(app.row, TERM_HORIZONTAL, TERM_FILL);
    print_reverse(rev);

    app.log = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_make_text(app.log, "");
    term_text_limit(app.log, 2000); /* fewer than 250 lines fit */
    term_text_autoscroll(app.log, true);

    app.progress = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_progress(app.progress, 1000);
    term_progress_set(app.progress, 999);

    /* Widgets checked through their API only; hidden so they take no space. */
    app.input = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_input(app.input);
    term_panel_show(app.input, false);
    app.list = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_list(app.list, items, 3);
    term_panel_show(app.list, false);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
