/*
 * Hardware test: overlays. A list with a status line; [zoom] opens a centered
 * "Rename" dialog over it, and the app moves focus into the dialog's input.
 * [enter] submits: the status line shows the name, the dialog closes, and
 * focus goes back to the list. [clear] in an empty dialog cancels it, leaving
 * the screen exactly as it was. [clear] on the list exits.
 */

#include "titrm.h"

static const char *const items[] = {"alpha", "beta", "gamma"};

typedef struct {
    term_panel_t *list;
    term_panel_t *status;
    term_panel_t *dialog; /* NULL when closed */
    term_panel_t *field;
} app_t;

static void open_dialog(term_ctx_t *ctx, app_t *app) {
    app->dialog = term_overlay_open_centered(ctx, 24, 3);
    term_panel_set_border(app->dialog, true);
    term_panel_set_title(app->dialog, "Rename");
    app->field = term_split(app->dialog, TERM_VERTICAL, TERM_FILL);
    term_make_input(app->field);
    term_focus(ctx, app->field);
}

static void close_dialog(app_t *app) {
    term_overlay_close(app->dialog); /* focus goes back to the list */
    app->dialog = NULL;
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->type == TERM_EV_SUBMIT && ev->panel == app->field) {
        term_panel_clear(app->status);
        term_panel_printf(app->status, "named %s", term_input_text(app->field));
        close_dialog(app);
        return true;
    }
    if (ev->type != TERM_EV_KEY) {
        return false;
    }
    if (ev->key == TERM_KEY_F3 && !app->dialog) {
        open_dialog(ctx, app);
        return true;
    }
    if (ev->key == TERM_KEY_CLEAR) {
        if (app->dialog) {
            close_dialog(app);
        } else {
            term_quit(ctx, 0);
        }
        return true;
    }
    return false;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    app.list = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(app.list, true);
    term_panel_set_title(app.list, "Items");
    term_make_list(app.list, items, 3);
    app.status = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_print(app.status, "[zoom] renames");
    term_focus(ctx, app.list);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
