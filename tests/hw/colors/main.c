/*
 * Hardware test: colors.
 *
 *   title    - a text widget, white on blue, centered
 *   swatches - each TERM_COLOR_* as a background, with its name
 *   box      - yellow on blue with a border and title: a list (focused, so
 *              its selection and title are reversed), a checkbox, a button,
 *              and a gap showing the box's own background
 *
 * [down] moves the list selection. [vars] moves focus to the next control, and
 * up/down do too when the focused control doesn't use them. [clear] exits.
 */

#include "titrm.h"

static const char *const items[] = {"Home", "Office", "Cafe"};

static const struct {
    const char *name;
    uint8_t color;
} swatches[] = {
    {"black", TERM_COLOR_BLACK},   {"white", TERM_COLOR_WHITE}, {"red", TERM_COLOR_RED},
    {"orange", TERM_COLOR_ORANGE}, {"yellow", TERM_COLOR_YELLOW}, {"green", TERM_COLOR_GREEN},
    {"blue", TERM_COLOR_BLUE},     {"purple", TERM_COLOR_PURPLE}, {"pink", TERM_COLOR_PINK},
};

static term_panel_t *order[3]; /* focus order: list, checkbox, button */

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type != TERM_EV_KEY) {
        return false;
    }
    int at = 0;
    for (int i = 0; i < 3; i++) {
        if (order[i] == term_focused(ctx)) {
            at = i;
        }
    }
    if (ev->key == TERM_KEY_VARS) {
        term_focus(ctx, order[(at + 1) % 3]);
    } else if (ev->key == TERM_KEY_DOWN && at < 2) {
        term_focus(ctx, order[at + 1]);
    } else if (ev->key == TERM_KEY_UP && at > 0) {
        term_focus(ctx, order[at - 1]);
    } else if (ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
    } else {
        return false;
    }
    return true;
}

int main(void) {
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    term_panel_t *title = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_set_colors(title, TERM_COLOR_WHITE, TERM_COLOR_BLUE);
    term_make_text(title, "colors");
    term_panel_set_align(title, TERM_ALIGN_CENTER);

    term_split(root, TERM_VERTICAL, TERM_FIXED(1)); /* spacer */
    term_panel_t *sw = term_split(root, TERM_VERTICAL, TERM_FIXED(9));
    for (unsigned i = 0; i < sizeof swatches / sizeof swatches[0]; i++) {
        uint8_t bg = swatches[i].color;
        term_panel_set_colors(sw, bg == TERM_COLOR_BLACK ? TERM_COLOR_WHITE : TERM_COLOR_BLACK, bg);
        term_panel_move(sw, 0, (int)i);
        term_panel_printf(sw, " %-10s", swatches[i].name);
    }

    term_split(root, TERM_VERTICAL, TERM_FIXED(1)); /* spacer */
    term_panel_t *box = term_split(root, TERM_VERTICAL, TERM_FIXED(10));
    term_panel_set_colors(box, TERM_COLOR_YELLOW, TERM_COLOR_BLUE);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "Box");
    term_panel_t *list = term_split(box, TERM_VERTICAL, TERM_FIXED(3));
    term_make_list(list, items, 3);
    term_split(box, TERM_VERTICAL, TERM_FIXED(1)); /* gap: the box's background */
    term_panel_t *check = term_split(box, TERM_VERTICAL, TERM_FIXED(1));
    term_make_checkbox(check, "Remember", true);
    term_split(box, TERM_VERTICAL, TERM_FIXED(1)); /* gap */
    term_panel_t *ok = term_split(box, TERM_VERTICAL, TERM_FIXED(1));
    term_make_button(ok, "OK");
    order[0] = list;
    order[1] = check;
    order[2] = ok;

    term_focus(ctx, list);
    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
