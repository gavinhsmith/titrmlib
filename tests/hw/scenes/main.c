/*
 * Hardware test: scenes. Two full-screen scenes, switched with [window]:
 *
 *   A: a status screen showing how many switches happened and the last item
 *      picked in B
 *   B: a list, focused by B's own handler whenever B is entered; [enter] on an
 *      item prints into A's panel while A isn't shown
 *
 * Switching back to B must show B exactly as it was (retained content, list
 * selection kept, focus restored by the scene handler). [clear] exits.
 */

#include "titrm.h"

static const char *const items[] = {"one", "two", "three"};

typedef struct {
    term_panel_t *a;
    term_panel_t *b;
    term_panel_t *status; /* in A */
    term_panel_t *picked; /* in A, written while B is shown */
    term_panel_t *list;   /* in B */
    int switches;
} app_t;

static void print_status(app_t *app) {
    term_panel_clear(app->status);
    term_panel_printf(app->status, "switches: %d", app->switches);
}

/* Scene B's handler: focus the list on entry, and take the list's [enter]. */
static bool scene_b(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->type == TERM_EV_SCENE_ENTER) {
        term_focus(ctx, app->list);
        return true;
    }
    if (ev->type == TERM_EV_SUBMIT && ev->panel == app->list) {
        term_panel_clear(app->picked);
        term_panel_printf(app->picked, "picked %s", items[ev->value]);
        return true;
    }
    return false;
}

/* The global handler: [window] switches scenes, [clear] quits. */
static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->type != TERM_EV_KEY) {
        return false;
    }
    if (ev->key == TERM_KEY_F2) {
        term_scene_switch(ctx, term_scene_active(ctx) == app->a ? app->b : app->a);
        app->switches++;
        print_status(app);
        return true;
    }
    if (ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
        return true;
    }
    return false;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();

    app.a = term_root(ctx);
    term_panel_t *box_a = term_split(app.a, TERM_VERTICAL, TERM_FIXED(6));
    term_panel_set_border(box_a, true);
    term_panel_set_title(box_a, "Scene A");
    app.status = term_split(box_a, TERM_VERTICAL, TERM_FIXED(1));
    app.picked = term_split(box_a, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *hint_a = term_split(box_a, TERM_VERTICAL, TERM_FILL);
    term_panel_print(hint_a, "[window] switches scene");
    term_panel_print(app.picked, "picked nothing");
    print_status(&app);

    app.b = term_scene_new(ctx, scene_b, &app);
    app.list = term_split(app.b, TERM_HORIZONTAL, TERM_FIXED(20));
    term_panel_set_border(app.list, true);
    term_panel_set_title(app.list, "Scene B");
    term_make_list(app.list, items, 3);
    term_panel_t *hint_b = term_split(app.b, TERM_HORIZONTAL, TERM_FILL);
    term_panel_print(hint_b, " [enter] picks");

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
