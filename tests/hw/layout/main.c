/*
 * Hardware test: layout. A nested panel tree using every size kind, checked
 * before and after the tree changes:
 *
 *   header (fixed 1)
 *   body: [30%] [fill: fixed 4 / weight 2 (L|R) / weight 1] [fixed 10]
 *   footer (fixed 1)
 *
 * [window] hides/shows the 30% panel, [zoom] destroys the weight-2 subtree,
 * [clear] exits. The "sizes" panel prints each panel's content size, so the
 * layout numbers are part of the screen being checked.
 */

#include "titrm.h"

typedef struct {
    term_panel_t *header;
    term_panel_t *footer;
    term_panel_t *sizes;
    term_panel_t *percent;
    term_panel_t *fixed;
    term_panel_t *weight2;
    term_panel_t *left;
    term_panel_t *weight1;
    bool destroyed;
} app_t;

static void print_header(term_panel_t *p) {
    term_panel_clear(p);
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_repeat(p, ' ', term_panel_width(p));
    term_panel_move(p, 1, 0);
    term_panel_print(p, "layout  [window] hide  [zoom] destroy");
}

static void print_footer(app_t *app) {
    term_panel_clear(app->footer);
    term_panel_printf(app->footer, "30%% %s, weight 2 %s",
                      term_panel_visible(app->percent) ? "shown" : "hidden",
                      app->destroyed ? "destroyed" : "present");
}

/* Everything past the panel's edges must be cut off. */
static void print_clip(term_panel_t *p) {
    term_panel_clear(p);
    term_panel_print(p, "clip: this line is much longer than the panel is wide");
    term_panel_move(p, 0, 1);
    term_panel_print(p, "row 1\nrow 2 (outside)\nrow 3 (outside)");
    term_panel_move(p, 200, 0);
    term_panel_print(p, "far right");
    term_panel_move(p, 0, 200);
    term_panel_print(p, "far below");
}

static void print_wrap(term_panel_t *p) {
    term_panel_clear(p);
    term_panel_wrap(p, true);
    term_panel_print(p, "wrap mode: this text continues on the next row when it reaches the edge.");
}

static void print_size(term_panel_t *out, char tag, const term_panel_t *p) {
    if (!p || !term_panel_visible(p)) {
        term_panel_printf(out, "%c  -\n", tag);
    } else {
        term_panel_printf(out, "%c %dx%d\n", tag, term_panel_width(p), term_panel_height(p));
    }
}

static void print_sizes(app_t *app) {
    term_panel_t *p = app->sizes;
    term_panel_clear(p);
    print_size(p, 'p', app->percent);
    print_size(p, 'f', app->fixed);
    print_size(p, '2', app->destroyed ? NULL : app->weight2);
    print_size(p, 'l', app->destroyed ? NULL : app->left);
    print_size(p, '1', app->weight1);
    print_size(p, 's', p);
}

/* Output is retained and doesn't reflow by itself, so the panels whose size
 * or text depends on the layout are printed again after every change. */
static void print_all(app_t *app) {
    print_header(app->header);
    print_footer(app);
    print_clip(app->fixed);
    print_wrap(app->weight1);
    print_sizes(app);
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    app_t *app = state;
    if (ev->type != TERM_EV_KEY) {
        return true;
    }
    switch (ev->key) {
    case TERM_KEY_F2:
        term_panel_show(app->percent, !term_panel_visible(app->percent));
        break;
    case TERM_KEY_F3:
        if (!app->destroyed) {
            term_panel_destroy(app->weight2);
            app->destroyed = true;
        }
        break;
    case TERM_KEY_CLEAR:
        term_quit(ctx, 0);
        return true;
    default:
        break;
    }
    print_all(app);
    return true;
}

int main(void) {
    static app_t app;
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    app.header = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *body = term_split(root, TERM_VERTICAL, TERM_FILL);
    app.footer = term_split(root, TERM_VERTICAL, TERM_FIXED(1));

    app.percent = term_split(body, TERM_HORIZONTAL, TERM_PERCENT(30));
    term_panel_t *mid = term_split(body, TERM_HORIZONTAL, TERM_FILL);
    app.sizes = term_split(body, TERM_HORIZONTAL, TERM_FIXED(10));

    term_panel_set_border(app.percent, true);
    term_panel_set_title(app.percent, "30%");
    term_make_text(app.percent, "A percent-sized panel. Its siblings take the space back when it is hidden.");

    app.fixed = term_split(mid, TERM_VERTICAL, TERM_FIXED(4));
    app.weight2 = term_split(mid, TERM_VERTICAL, TERM_FILL_WEIGHT(2));
    app.weight1 = term_split(mid, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(app.fixed, true);
    term_panel_set_title(app.fixed, "fixed 4");
    term_panel_set_border(app.weight2, true);
    term_panel_set_title(app.weight2, "weight 2");
    term_panel_set_border(app.weight1, true);
    term_panel_set_title(app.weight1, "weight 1");

    /* Two more levels inside weight 2. */
    app.left = term_split(app.weight2, TERM_HORIZONTAL, TERM_FILL);
    term_panel_t *right = term_split(app.weight2, TERM_HORIZONTAL, TERM_FILL);
    term_panel_set_border(app.left, true);
    term_panel_set_title(app.left, "L");
    term_make_text(app.left, "Left of a nested split.");
    term_panel_t *r_top = term_split(right, TERM_VERTICAL, TERM_FIXED(3));
    term_panel_t *r_bottom = term_split(right, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(r_top, true);
    term_panel_set_title(r_top, "R1");
    term_make_text(r_top, "depth 4");
    term_panel_set_border(r_bottom, true);
    term_panel_set_title(r_bottom, "R2");
    term_make_text(r_bottom, TERM_S_ARROW_R " deepest");

    term_panel_set_border(app.sizes, true);
    term_panel_set_title(app.sizes, "sizes");

    print_all(&app);

    term_run(ctx, on_event, &app);
    term_shutdown(ctx);
    return 0;
}
