/*
 * Hardware test: ticks. term_set_tick() drives a progress bar through 20
 * ticks of 100 ms using the real clock(), then stops ticking and reports
 * whether the 20 ticks took a plausible time. The final screen is the same
 * every run; only a timing failure changes it (and shows the elapsed time).
 *
 * [clear] exits.
 */

#include <stdio.h>
#include <time.h>

#include "titrm.h"

#define TICKS 20
#define TICK_MS 100

typedef struct {
    term_panel_t *progress;
    term_panel_t *info;
    int ticks;
    clock_t start;
    char text[64];
} app_t;

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    switch (ev->type) {
    case TERM_EV_START:
        app->start = clock();
        term_set_tick(ctx, TICK_MS);
        break;
    case TERM_EV_TICK:
        app->ticks++;
        term_progress_set(app->progress, app->ticks);
        if (app->ticks == TICKS) {
            term_set_tick(ctx, 0);
            unsigned long ms = (unsigned long)(clock() - app->start) * 1000 / CLOCKS_PER_SEC;
            /* Each tick fires on the first poll after it is due, so a little
             * over TICKS * TICK_MS is expected; well under or over is a bug. */
            if (ms >= TICKS * TICK_MS && ms < TICKS * TICK_MS * 3 / 2) {
                sprintf(app->text, "%d ticks of %d ms: timing ok", TICKS, TICK_MS);
            } else {
                sprintf(app->text, "%d ticks of %d ms: timing OFF (%lu ms)", TICKS, TICK_MS, ms);
            }
        } else {
            sprintf(app->text, "tick %d of %d", app->ticks, TICKS);
        }
        term_text_set(app->info, app->text);
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
    term_panel_t *root = term_root(ctx);

    term_panel_t *box = term_split(root, TERM_VERTICAL, TERM_FIXED(5));
    term_panel_set_border(box, true);
    term_panel_set_title(box, "ticks");
    app.progress = term_split(box, TERM_VERTICAL, TERM_FIXED(1));
    term_make_progress(app.progress, TICKS);
    term_split(box, TERM_VERTICAL, TERM_FIXED(1)); /* spacer */
    app.info = term_split(box, TERM_VERTICAL, TERM_FILL);
    sprintf(app.text, "waiting for the first tick");
    term_make_text(app.info, app.text);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
