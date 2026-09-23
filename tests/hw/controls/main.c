/*
 * Hardware test: controls. The newer widgets and the pieces for building
 * custom ones:
 *
 *   title   - a text widget, centered and reversed (a status bar)
 *   2 checkboxes
 *   counter - a custom widget: left/right change it, it sends TERM_EV_CHANGE
 *   OK / Cancel buttons side by side
 *   status  - the last event the app heard
 *
 * The app moves focus with [vars]. [clear] exits.
 */

#include "titrm.h"

typedef struct {
    term_panel_t *status;
    term_panel_t *order[5]; /* focus order */
    term_panel_t *auto_box;
    term_panel_t *counter;
    term_panel_t *ok;
    int value;
} app_t;

static void show_counter(app_t *app) {
    term_panel_clear(app->counter);
    term_panel_printf(app->counter, "counter: %d  (left/right)", app->value);
}

static bool counter_keys(term_panel_t *p, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->key != TERM_KEY_LEFT && ev->key != TERM_KEY_RIGHT) {
        return false;
    }
    app->value += ev->key == TERM_KEY_RIGHT ? 1 : -1;
    show_counter(app);
    term_panel_send(p, TERM_EV_CHANGE, app->value);
    return true;
}

static void set_status(app_t *app, const char *what) {
    term_text_set(app->status, what);
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->type == TERM_EV_CHANGE) {
        term_text_clear(app->status);
        term_text_appendf(app->status, "%s %d", ev->panel == app->counter ? "counter" : "checkbox",
                          ev->value);
        return true;
    }
    if (ev->type == TERM_EV_SUBMIT) {
        set_status(app, ev->panel == app->ok ? "submit OK" : "submit Cancel");
        return true;
    }
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_VARS) {
        int next = 0;
        for (int i = 0; i < 5; i++) {
            if (app->order[i] == term_focused(ctx)) {
                next = (i + 1) % 5;
            }
        }
        term_focus(ctx, app->order[next]);
        return true;
    }
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
        return true;
    }
    return false;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    term_panel_t *title = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_text(title, "controls");
    term_panel_set_align(title, TERM_ALIGN_CENTER);
    term_panel_set_attr(title, TERM_ATTR_REVERSE);

    term_split(root, TERM_VERTICAL, TERM_FIXED(1)); /* spacer */
    app.auto_box = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_checkbox(app.auto_box, "Auto-connect", false);
    term_panel_t *remember = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_make_checkbox(remember, "Remember network", true);

    app.counter = term_split(root, TERM_VERTICAL, TERM_FIXED(2));
    term_panel_set_focusable(app.counter, true);
    term_panel_set_keys(app.counter, counter_keys, &app);
    show_counter(&app);

    term_panel_t *buttons = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    app.ok = term_split(buttons, TERM_HORIZONTAL, TERM_FILL);
    term_split(buttons, TERM_HORIZONTAL, TERM_FIXED(2)); /* gap */
    term_panel_t *cancel = term_split(buttons, TERM_HORIZONTAL, TERM_FILL);
    term_make_button(app.ok, "OK");
    term_make_button(cancel, "Cancel");

    term_split(root, TERM_VERTICAL, TERM_FIXED(1)); /* spacer */
    app.status = term_split(root, TERM_VERTICAL, TERM_FILL);
    term_make_text(app.status, "no events yet");

    term_panel_t *order[5] = {app.auto_box, remember, app.counter, app.ok, cancel};
    for (int i = 0; i < 5; i++) {
        app.order[i] = order[i];
    }
    term_focus(ctx, app.auto_box);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
