/*
 * Hardware test: glyphs. One static screen showing every character code, the
 * reverse attribute, box-drawing joins and word-wrapped text, so the font
 * table and the glyph blit are checked pixel for pixel on the real graphx.
 *
 *   +- Glyphs -----------------------+ +- Reverse ---+
 *   |    0 1 2 ... F                 | +- Joins -----+
 *   | 0x . . .                       | |             |
 *   +--------------------------------+ +-------------+
 *   +- Text ---------------------------------------- +
 *
 * [clear] exits.
 */

#include "titrm.h"

static void draw_table(term_ctx_t *ctx, term_panel_t *p, void *user) {
    (void)ctx;
    (void)user;
    term_panel_print(p, "   0 1 2 3 4 5 6 7 8 9 A B C D E F");
    for (int hi = 0; hi < 16; hi++) {
        term_panel_move(p, 0, hi + 1);
        term_panel_printf(p, "%Xx ", hi);
        for (int lo = 0; lo < 16; lo++) {
            char c = (char)(hi << 4 | lo);
            /* putc treats these two as line control, not glyphs */
            term_panel_putc(p, (c == '\n' || c == '\r') ? ' ' : c);
            term_panel_putc(p, ' ');
        }
    }
}

static void draw_reverse(term_ctx_t *ctx, term_panel_t *p, void *user) {
    (void)ctx;
    (void)user;
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_print(p, "ABC abc 123\n");
    term_panel_print(p, TERM_S_CHECK TERM_S_CROSSMARK TERM_S_DOT TERM_S_DOT_EMPTY
                        TERM_S_ARROW_R TERM_S_ARROW_D TERM_S_ARROW_U TERM_S_SMILE "\n");
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
    term_panel_print(p, "norm");
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_print(p, "rev");
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
    term_panel_print(p, "norm\n");
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_repeat(p, ' ', term_panel_width(p));
}

static void draw_joins(term_ctx_t *ctx, term_panel_t *p, void *user) {
    (void)ctx;
    (void)user;
    term_panel_print(p,
                     TERM_S_TL TERM_S_HLINE TERM_S_HLINE TERM_S_TTEE TERM_S_HLINE TERM_S_HLINE TERM_S_TR "\n"
                     TERM_S_VLINE "AB" TERM_S_VLINE "CD" TERM_S_VLINE "\n"
                     TERM_S_LTEE TERM_S_HLINE TERM_S_HLINE TERM_S_CROSS TERM_S_HLINE TERM_S_HLINE TERM_S_RTEE "\n"
                     TERM_S_VLINE "EF" TERM_S_VLINE "GH" TERM_S_VLINE "\n"
                     TERM_S_BL TERM_S_HLINE TERM_S_HLINE TERM_S_BTEE TERM_S_HLINE TERM_S_HLINE TERM_S_BR "\n"
                     "\n"
                     TERM_S_SIG0 TERM_S_SIG1 TERM_S_SIG2 TERM_S_SIG3 " signal\n"
                     TERM_S_SHADE TERM_S_SHADE TERM_S_BLOCK TERM_S_BLOCK TERM_S_SHADE TERM_S_SHADE " fill");
}

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
    }
}

int main(void) {
    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    term_panel_t *top = term_split(root, TERM_VERTICAL, TERM_FIXED(19));
    term_panel_t *text = term_split(root, TERM_VERTICAL, TERM_FILL);

    term_panel_t *table = term_split(top, TERM_HORIZONTAL, TERM_FIXED(37));
    term_panel_t *side = term_split(top, TERM_HORIZONTAL, TERM_FILL);
    term_panel_set_border(table, true);
    term_panel_set_title(table, "Glyphs");
    term_panel_set_draw(table, draw_table, NULL);

    term_panel_t *reverse = term_split(side, TERM_VERTICAL, TERM_FIXED(6));
    term_panel_t *joins = term_split(side, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(reverse, true);
    term_panel_set_title(reverse, "Reverse");
    term_panel_set_draw(reverse, draw_reverse, NULL);
    term_panel_set_border(joins, true);
    term_panel_set_title(joins, "Joins");
    term_panel_set_draw(joins, draw_joins, NULL);

    term_panel_set_border(text, true);
    term_panel_set_title(text, "Text");
    term_make_text(text,
                   "The quick brown fox jumps over the lazy dog. "
                   "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.\n"
                   "0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\n"
                   "  Indented, then a word too long for one line: "
                   "Supercalifragilisticexpialidocious-and-then-some-more-letters.");

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
