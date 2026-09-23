/*
 * Hardware test: perf. Times frames through the public API: a 1 ms tick
 * makes term_run() draw a frame per loop, so the gap between ticks is one
 * frame. Three cases, 20 frames each:
 *
 *   idle  - the draw callback repeats the same full screen (nothing to flush)
 *   full  - every cell changes each frame (two alternating screens of text)
 *   line  - one row changes each frame (typical: a list selection moving)
 *
 * The screen reports each case against its budget, so it is the same every
 * run unless a budget is missed; the measured times are shown only then.
 * [clear] exits.
 */

#include <stdio.h>
#include <time.h>

#include "titrm.h"

#define FRAMES 20

enum { CASE_IDLE, CASE_FULL, CASE_LINE, NUM_CASES };

static const char *const names[NUM_CASES] = {"idle", "full", "line"};
static const unsigned budget_ms[NUM_CASES] = {100, 500, 130};

typedef struct {
    term_panel_t *screen;
    int which; /* case being timed, NUM_CASES when done */
    int frame;
    clock_t start;
    unsigned ms[NUM_CASES];
    char report[NUM_CASES * 48];
} app_t;

static void draw_screen(term_ctx_t *ctx, term_panel_t *p, void *user) {
    (void)ctx;
    app_t *app = user;
    if (app->which == NUM_CASES) {
        term_panel_print(p, app->report);
        return;
    }
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    for (int r = 0; r < h; r++) {
        term_panel_move(p, 0, r);
        char c = 'A' + r % 26;
        if (app->which == CASE_FULL && app->frame % 2) {
            c = 'a' + r % 26;
        } else if (app->which == CASE_LINE && r == app->frame % h) {
            term_panel_set_attr(p, TERM_ATTR_REVERSE);
        }
        term_panel_repeat(p, c, w);
        term_panel_set_attr(p, TERM_ATTR_NORMAL);
    }
}

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    switch (ev->type) {
    case TERM_EV_START:
        term_set_tick(ctx, 1);
        break;
    case TERM_EV_TICK:
        if (app->frame == 0) {
            app->start = clock();
        }
        if (++app->frame <= FRAMES) {
            break;
        }
        app->ms[app->which] =
            (unsigned)((unsigned long)(clock() - app->start) * 1000 / CLOCKS_PER_SEC / FRAMES);
        app->frame = 0;
        if (++app->which == NUM_CASES) {
            term_set_tick(ctx, 0);
            char *out = app->report;
            for (int i = 0; i < NUM_CASES; i++) {
                if (app->ms[i] <= budget_ms[i]) {
                    out += sprintf(out, "%s frame under %u ms: ok\n", names[i], budget_ms[i]);
                } else {
                    out += sprintf(out, "%s frame under %u ms: SLOW (%u ms)\n", names[i], budget_ms[i],
                                   app->ms[i]);
                }
            }
        }
        break;
    case TERM_EV_KEY:
        if (ev->key == TERM_KEY_CLEAR) {
            term_quit(ctx, 0);
        }
        break;
    default:
        break;
    }
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    app.screen = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_set_draw(app.screen, draw_screen, &app);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
