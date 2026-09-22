/* The smallest useful titrmlib program: one bordered panel with some text. */

#include "titrm.h"

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
    }
}

int main(void) {
    term_ctx_t *ctx = term_init();

    term_panel_t *box = term_split(term_root(ctx), TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "titrmlib");
    term_make_text(box,
                   "Hello from titrmlib!\n"
                   "\n"
                   "This whole screen is a " TERM_S_CHECK " character grid "
                   "drawn with a 5x7 font.\n"
                   "\n"
                   "Press [clear] to quit.");

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
