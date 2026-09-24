/*
 * Hardware test: styles. Every text style alone and combined, in color and
 * reversed, over box drawing (which ignores them), inline in a text widget,
 * and as a list's focus highlight; tab stops in panels, text and list items.
 *
 *   +- Styles ----------------------+ +- Joins -----+
 *   | normal     Hello, world       | |             |
 *   | ...                           | +- Tabs ------+
 *   +-------------------------------+ |             |
 *   +- Inline ----------------------+ +- List ------+
 *
 * [down] moves the list selection; [clear] exits.
 */

#include "titrm.h"

static const struct {
    const char *name;
    uint8_t attr;
} styles[] = {
    {"normal", TERM_ATTR_NORMAL},
    {"bold", TERM_ATTR_BOLD},
    {"italic", TERM_ATTR_ITALIC},
    {"underline", TERM_ATTR_UNDERLINE},
    {"strike", TERM_ATTR_STRIKE},
    {"bold+ital", TERM_ATTR_BOLD | TERM_ATTR_ITALIC},
    {"all four", TERM_ATTR_BOLD | TERM_ATTR_ITALIC | TERM_ATTR_UNDERLINE | TERM_ATTR_STRIKE},
    {"rev+under", TERM_ATTR_REVERSE | TERM_ATTR_UNDERLINE},
};

static void print_styles(term_panel_t *p) {
    for (int i = 0; i < (int)(sizeof styles / sizeof styles[0]); i++) {
        term_panel_set_attr(p, TERM_ATTR_NORMAL);
        term_panel_move(p, 0, i);
        term_panel_print(p, styles[i].name);
        term_panel_move(p, 11, i);
        term_panel_set_attr(p, styles[i].attr);
        term_panel_print(p, "Hello, world! gjpqy");
    }
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
}

static void print_joins(term_panel_t *p) {
    /* The same box twice: styles must not change box drawing. */
    for (int i = 0; i < 2; i++) {
        term_panel_set_attr(p, i ? TERM_ATTR_BOLD | TERM_ATTR_ITALIC | TERM_ATTR_UNDERLINE
                                 : TERM_ATTR_NORMAL);
        term_panel_print(p, TERM_S_TL TERM_S_HLINE TERM_S_TTEE TERM_S_HLINE TERM_S_TR "\n"
                            TERM_S_VLINE "A" TERM_S_VLINE "B" TERM_S_VLINE "\n"
                            TERM_S_BL TERM_S_HLINE TERM_S_BTEE TERM_S_HLINE TERM_S_BR "\n");
    }
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
}

static void print_tabs(term_panel_t *p) {
    term_panel_print(p, "a\tbb\tccc\td\n"
                        "1234\t5\t67\n"
                        "\t" TERM_S_UNDERLINE "under" TERM_S_NORMAL "\tx");
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
        return true;
    }
    return false;
}

int main(void) {
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    term_panel_t *top = term_split(root, TERM_VERTICAL, TERM_FIXED(21));
    term_panel_t *bottom = term_split(root, TERM_VERTICAL, TERM_FILL);

    term_panel_t *left = term_split(top, TERM_HORIZONTAL, TERM_FIXED(34));
    term_panel_t *side = term_split(top, TERM_HORIZONTAL, TERM_FILL);
    term_panel_t *plain = term_split(left, TERM_VERTICAL, TERM_FIXED(10));
    term_panel_t *color = term_split(left, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(plain, true);
    term_panel_set_title(plain, "Styles");
    term_panel_set_border(color, true);
    term_panel_set_title(color, "In color");
    term_panel_set_colors(color, TERM_COLOR_YELLOW, TERM_COLOR_BLUE);

    term_panel_t *joins = term_split(side, TERM_VERTICAL, TERM_FIXED(8));
    term_panel_t *tabs = term_split(side, TERM_VERTICAL, TERM_FIXED(5));
    term_panel_t *list = term_split(side, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(joins, true);
    term_panel_set_title(joins, "Joins");
    term_panel_set_border(tabs, true);
    term_panel_set_title(tabs, "Tabs");
    term_panel_set_border(list, true);
    term_panel_set_title(list, "List");

    static const char *const items[] = {
        "one\t" TERM_S_BOLD "1",
        "two\t" TERM_S_ITALIC "2",
        "three\t" TERM_S_STRIKE "3",
    };
    term_make_list(list, items, 3);
    term_panel_set_focus_attr(list, TERM_ATTR_BOLD | TERM_ATTR_UNDERLINE);
    term_focus(ctx, list);

    term_panel_set_border(bottom, true);
    term_panel_set_title(bottom, "Inline");
    term_make_text(bottom,
                   "Words in " TERM_S_BOLD "bold" TERM_S_NORMAL ", " TERM_S_ITALIC "italic"
                   TERM_S_NORMAL ", " TERM_S_UNDERLINE "underlined across words" TERM_S_NORMAL
                   " and " TERM_S_STRIKE "struck" TERM_S_NORMAL ", wrapping mid-" TERM_S_BOLD
                   "styled-run-that-keeps-going" TERM_S_NORMAL " onto the next row.\n"
                   "name\tsize\tok\n"
                   "a.txt\t12\t" TERM_S_BOLD "yes\n"
                   "styles end with the line.");

    print_styles(plain);
    term_panel_print(color, "bold\t\t" TERM_S_BOLD "Colored text\n" TERM_S_NORMAL
                            "under\t\t" TERM_S_UNDERLINE "Colored text\n" TERM_S_NORMAL
                            "reverse\t\t" TERM_S_REVERSE "Colored text" TERM_S_NORMAL);
    print_joins(joins);
    print_tabs(tabs);

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
