/*
 * Hardware test: perf. Times updates through the public API: a 1 ms tick
 * gives the app a chance to change the screen on every loop, so the gap
 * between ticks is the app's printing plus one frame. Three cases, 20 ticks
 * each:
 *
 *   idle  - the app changes nothing (the frame is skipped)
 *   line  - a highlight moves down one row (typical: a list selection moving)
 *   full  - every cell changes (two alternating screens of text)
 *
 * The screen reports each case against its budget, so it is the same every
 * run unless a budget is missed; the measured times are shown only then.
 * [clear] exits.
 */

#include <stdio.h>
#include <time.h>

#include "titrm.h"

#define FRAMES 20

enum { CASE_IDLE, CASE_LINE, CASE_FULL, NUM_CASES };

static const char *const names[NUM_CASES] = {"idle", "line", "full"};
static const unsigned budget_ms[NUM_CASES] = {5, 50, 600};

typedef struct {
    term_panel_t *screen;
    int which; /* case being timed, NUM_CASES when done */
    int frame;
    clock_t start;
    unsigned ms[NUM_CASES];
    char report[NUM_CASES * 48];
} app_t;

/* Row `r` of the test pattern: a letter across the full width. */
static void print_row(term_panel_t *p, int r, char c, bool highlight) {
    term_panel_move(p, 0, r);
    term_panel_set_attr(p, highlight ? TERM_ATTR_REVERSE : TERM_ATTR_NORMAL);
    term_panel_repeat(p, c, term_panel_width(p));
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
}

static void print_screen(term_panel_t *p, bool lower) {
    for (int r = 0; r < term_panel_height(p); r++) {
        print_row(p, r, (lower ? 'a' : 'A') + r % 26, false);
    }
}

/* One tick's worth of change for the case being timed. */
static void update(app_t *app) {
    term_panel_t *p = app->screen;
    int h = term_panel_height(p);
    if (app->which == CASE_LINE) {
        int r = app->frame % h;
        int prev = (r + h - 1) % h;
        print_row(p, prev, 'A' + prev % 26, false);
        print_row(p, r, 'A' + r % 26, true);
    } else if (app->which == CASE_FULL) {
        print_screen(p, app->frame % 2);
    }
}

static void report(app_t *app) {
    char *out = app->report;
    for (int i = 0; i < NUM_CASES; i++) {
        if (app->ms[i] <= budget_ms[i]) {
            out += sprintf(out, "%s update under %u ms: ok\n", names[i], budget_ms[i]);
        } else {
            out += sprintf(out, "%s update under %u ms: SLOW (%u ms)\n", names[i], budget_ms[i],
                           app->ms[i]);
        }
    }
    term_panel_clear(app->screen);
    term_panel_print(app->screen, app->report);
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
            update(app);
            break;
        }
        app->ms[app->which] =
            (unsigned)((unsigned long)(clock() - app->start) * 1000 / CLOCKS_PER_SEC / FRAMES);
        app->frame = 0;
        if (++app->which == NUM_CASES) {
            term_set_tick(ctx, 0);
            report(app);
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
    print_screen(app.screen, false);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
