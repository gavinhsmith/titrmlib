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

static void print_table(term_panel_t *p) {
    term_panel_print(p, "   0 1 2 3 4 5 6 7 8 9 A B C D E F");
    for (int hi = 0; hi < 16; hi++) {
        term_panel_move(p, 0, hi + 1);
        term_panel_printf(p, "%Xx ", hi);
        for (int lo = 0; lo < 16; lo++) {
            char c = (char)(hi << 4 | lo);
            /* putc treats these as control codes, not glyphs */
            term_panel_putc(p, (c == '\n' || c == '\r' || c == '\x1b') ? ' ' : c);
            term_panel_putc(p, ' ');
        }
    }
}

static void print_reverse(term_panel_t *p) {
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_print(p, "ABC abc 123\n");
    term_panel_print(p, "\x03\x04\x05\x06" TERM_S_ARROW_R TERM_S_ARROW_D TERM_S_ARROW_U
                        TERM_S_SMILE "\x82\x8E\xA4\xE1\xE3\xF1\n");
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
    term_panel_print(p, "norm");
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_print(p, "rev");
    term_panel_set_attr(p, TERM_ATTR_NORMAL);
    term_panel_print(p, "norm\n");
    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_repeat(p, ' ', term_panel_width(p));
}

static void print_joins(term_panel_t *p) {
    term_panel_print(p,
                     TERM_S_TL TERM_S_HLINE TERM_S_HLINE TERM_S_TTEE TERM_S_HLINE TERM_S_HLINE TERM_S_TR "\n"
                     TERM_S_VLINE "AB" TERM_S_VLINE "CD" TERM_S_VLINE "\n"
                     TERM_S_LTEE TERM_S_HLINE TERM_S_HLINE TERM_S_CROSS TERM_S_HLINE TERM_S_HLINE TERM_S_RTEE "\n"
                     TERM_S_VLINE "EF" TERM_S_VLINE "GH" TERM_S_VLINE "\n"
                     TERM_S_BL TERM_S_HLINE TERM_S_HLINE TERM_S_BTEE TERM_S_HLINE TERM_S_HLINE TERM_S_BR "\n"
                     "\xC9\xCD\xCD\xD1\xCD\xCD\xBB\n" /* double lines, meeting single ones */
                     "\xBA" "AB" "\xB3" "CD" "\xBA\n"
                     "\xC7\xC4\xC4\xC5\xC4\xC4\xB6\n"
                     "\xC8\xCD\xCD\xCF\xCD\xCD\xBC\n"
                     TERM_S_SIG0 TERM_S_SIG1 TERM_S_SIG2 TERM_S_SIG3 " signal\n"
                     "\xB0\xB0\xB1\xB1\xB2\xB2\xDB\xDB\xDC\xDC\xDF\xDF\xDD\xDE");
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    (void)state;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR) {
        term_quit(ctx, 0);
    }
    return true;
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

    term_panel_t *reverse = term_split(side, TERM_VERTICAL, TERM_FIXED(6));
    term_panel_t *joins = term_split(side, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(reverse, true);
    term_panel_set_title(reverse, "Reverse");
    term_panel_set_border(joins, true);
    term_panel_set_title(joins, "Joins");

    term_panel_set_border(text, true);
    term_panel_set_title(text, "Text");
    term_make_text(text,
                   "The quick brown fox jumps over the lazy dog. "
                   "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.\n"
                   "0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\n"
                   "  Indented, then a word too long for one line: "
                   "Supercalifragilisticexpialidocious-and-then-some-more-letters.");

    /* Output is retained: printed once, shown every frame. */
    print_table(table);
    print_reverse(reverse);
    print_joins(joins);

    term_run(ctx, on_event, NULL);
    term_shutdown(ctx);
    return 0;
}
