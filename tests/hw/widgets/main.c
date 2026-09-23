/*
 * Hardware test: widgets and input. Real keypad presses (from the autotester)
 * drive a list, an input field and a log, with focus moved by [vars]:
 *
 *   status: focused panel, alpha state
 *   +- List -------+ +- Input ------------------------------+
 *   | > item 1     | +- Log --------------------------------+
 *   |   ...        | | boot line 1 ...                      |
 *   +--------------+ +--------------------------------------+
 *   footer: selection, input text
 *
 * Every event the app gets is written to the log, so the screen records what
 * happened. [clear] (with an empty input) exits.
 */

#include <stdio.h>

#include "titrm.h"

#define NUM_ITEMS 30

typedef struct {
    term_panel_t *status;
    term_panel_t *footer;
    term_panel_t *list;
    term_panel_t *input;
    term_panel_t *log;
    const char *items[NUM_ITEMS];
    char labels[NUM_ITEMS][12];
} app_t;

static const char *focus_name(const app_t *app, const term_panel_t *p) {
    if (p == app->list) return "list";
    if (p == app->input) return "input";
    if (p == app->log) return "log";
    return p ? "?" : "none";
}

static void print_status(term_ctx_t *ctx, app_t *app) {
    term_panel_t *p = app->status;
    term_panel_clear(p);
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_repeat(p, ' ', term_panel_width(p));
    term_panel_move(p, 1, 0);
    term_panel_printf(p, "widgets  focus:%s  alpha:%d", focus_name(app, term_focused(ctx)),
                      term_alpha_mode(ctx));
}

static void print_footer(app_t *app) {
    term_panel_clear(app->footer);
    term_panel_printf(app->footer, "sel=%d input=\"%s\"", term_list_selected(app->list),
                      term_input_text(app->input));
}

/* [vars] moves focus list -> input -> log -> list. */
static void focus_next(term_ctx_t *ctx, app_t *app) {
    term_panel_t *now = term_focused(ctx);
    term_focus(ctx, now == app->list ? app->input : now == app->input ? app->log : app->list);
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    switch (ev->type) {
    case TERM_EV_START:
        for (int i = 1; i <= 30; i++) {
            term_text_appendf(app->log, "boot line %d\n", i);
        }
        break;
    case TERM_EV_SUBMIT:
        if (ev->panel == app->list) {
            term_text_appendf(app->log, "select %d (%s)\n", ev->value, app->items[ev->value]);
        } else {
            term_text_appendf(app->log, "submit \"%s\"\n", term_input_text(ev->panel));
            term_input_set(ev->panel, "");
        }
        break;
    case TERM_EV_KEY:
        if (ev->key == TERM_KEY_CLEAR) {
            term_quit(ctx, 0);
            return true;
        } else if (ev->key == TERM_KEY_VARS) {
            focus_next(ctx, app);
        } else if (ev->key != TERM_KEY_ALPHA) {
            term_text_appendf(app->log, "key %d from %s\n", (int)ev->key, focus_name(app, ev->panel));
        }
        break;
    default: /* TERM_EV_CHANGE: the footer shows the new value */
        break;
    }
    print_status(ctx, app);
    print_footer(app);
    return true;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    for (int i = 0; i < NUM_ITEMS; i++) {
        /* A few items lead with icons to check glyphs in reverse video. */
        const char *icon = i % 3 == 0 ? TERM_S_CHECK : (i % 3 == 1 ? TERM_S_SIG2 : "");
        sprintf(app.labels[i], "%sitem %d", icon, i + 1);
        app.items[i] = app.labels[i];
    }

    app.status = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *body = term_split(root, TERM_VERTICAL, TERM_FILL);
    app.footer = term_split(root, TERM_VERTICAL, TERM_FIXED(1));

    app.list = term_split(body, TERM_HORIZONTAL, TERM_FIXED(16));
    term_panel_t *right = term_split(body, TERM_HORIZONTAL, TERM_FILL);
    term_panel_set_border(app.list, true);
    term_panel_set_title(app.list, "List");
    term_make_list(app.list, app.items, NUM_ITEMS);

    app.input = term_split(right, TERM_VERTICAL, TERM_FIXED(3));
    app.log = term_split(right, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(app.input, true);
    term_panel_set_title(app.input, "Input");
    term_make_input(app.input);
    term_panel_set_border(app.log, true);
    term_panel_set_title(app.log, "Log");
    term_make_text(app.log, "");
    term_text_autoscroll(app.log, true);
    term_panel_set_focusable(app.log, true);

    term_focus(ctx, app.list);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
