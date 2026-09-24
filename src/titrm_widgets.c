/* Widgets: panels with built-in content and key handling. Each one draws
 * itself into its panel's retained cells through the same clipped term_put()
 * the app's output uses. Changing a widget's state calls term_panel_touch(),
 * and the framework redraws it before the next frame. */

#include "titrm_internal.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_LIMIT 1024 /* default term_text_limit() */

/* Bytes of an inline style escape at `s` (see TERM_S_*): 2, or 1 for an ESC
 * without its argument, which is dropped; 0 if `s` isn't an escape. */
static int esc_len(const char *s) {
    if (*s != TERM_ESC) {
        return 0;
    }
    return TERM_IS_ESC_ARG(s[1]) ? 2 : 1;
}

static void put_str(term_panel_t *p, int col, int row, const char *s, int max, uint8_t attr) {
    uint8_t style = 0;
    for (int i = 0; *s && i < max;) {
        int e = esc_len(s);
        if (e) {
            if (e == 2) {
                style = s[1] & 0x1F;
            }
            s += e;
            continue;
        }
        if (*s == '\t') { /* spaces to the next stop */
            do {
                term_put(p, col + i++, row, ' ', attr | style);
            } while (i < max && (i & (TERM_TAB - 1)));
            s++;
            continue;
        }
        term_put(p, col + i++, row, (uint8_t)*s++, attr | style);
    }
}

static void fill_row(term_panel_t *p, int row, int from, int to, uint8_t attr) {
    for (int c = from; c < to; c++) {
        term_put(p, c, row, 0, attr);
    }
}

static bool focused(const term_panel_t *p) {
    return p == p->ctx->focus;
}

/* A widget takes over a leaf panel; panels with children stay containers. */
static bool begin_widget(term_panel_t *p, term_kind_t kind, bool focusable) {
    if (!p || p->first_child) {
        return false;
    }
    term_widget_free(p);
    memset(&p->u, 0, sizeof p->u);
    p->kind = kind;
    p->focusable = focusable;
    term_panel_touch(p);
    return true;
}

static bool is_text(const term_panel_t *p) {
    return p->kind == TERM_KIND_TEXT || p->kind == TERM_KIND_BUTTON || p->kind == TERM_KIND_CHECKBOX;
}

/* ---- Text ---------------------------------------------------------------- */

/* Drops whole lines from the front until `need` more bytes fit the limit. */
static void text_make_room(term_panel_t *p, size_t need) {
    size_t len = p->u.text.len;
    while (len && len + need > p->u.text.limit) {
        char *nl = memchr(p->u.text.buf, '\n', len);
        size_t drop = nl ? (size_t)(nl - p->u.text.buf) + 1 : len;
        memmove(p->u.text.buf, p->u.text.buf + drop, len - drop);
        len -= drop;
    }
    p->u.text.len = (uint16_t)len;
}

void term_text_append(term_panel_t *p, const char *text) {
    if (!is_text(p)) {
        return;
    }
    size_t n = strlen(text);
    if (n > p->u.text.limit) { /* more than fits at all: keep the end */
        text += n - p->u.text.limit;
        n = p->u.text.limit;
    }
    text_make_room(p, n);
    char *buf = realloc(p->u.text.buf, p->u.text.len + n + 1);
    if (!buf) {
        return;
    }
    memcpy(buf + p->u.text.len, text, n);
    p->u.text.buf = buf;
    p->u.text.len += (uint16_t)n;
    buf[p->u.text.len] = '\0';
    term_panel_touch(p);
}

static void out_count(void *dst, char c) {
    (void)c;
    ++*(size_t *)dst;
}

static void out_buf(void *dst, char c) {
    *(*(char **)dst)++ = c;
}

void term_text_appendf(term_panel_t *p, const char *fmt, ...) {
    if (!is_text(p)) {
        return;
    }
    va_list args, again;
    va_start(args, fmt);
    va_copy(again, args);
    size_t n = 0;
    term_vformat(out_count, &n, fmt, args);
    char *buf = malloc(n + 1);
    if (buf) {
        char *end = buf;
        term_vformat(out_buf, &end, fmt, again);
        *end = '\0';
        term_text_append(p, buf);
        free(buf);
    }
    va_end(again);
    va_end(args);
}

void term_text_clear(term_panel_t *p) {
    if (is_text(p)) {
        p->u.text.len = 0;
        p->u.text.top = 0;
        if (p->u.text.buf) {
            p->u.text.buf[0] = '\0';
        }
        term_panel_touch(p);
    }
}

void term_text_set(term_panel_t *p, const char *text) {
    term_text_clear(p);
    term_text_append(p, text);
}

static void text_init(term_panel_t *p, const char *text) {
    p->u.text.limit = TEXT_LIMIT;
    term_text_append(p, text);
}

void term_make_text(term_panel_t *p, const char *text) {
    if (begin_widget(p, TERM_KIND_TEXT, false)) {
        text_init(p, text);
    }
}

void term_text_limit(term_panel_t *p, int bytes) {
    if (is_text(p)) {
        p->u.text.limit = bytes < 1 ? 1 : (bytes > 0xFFFF ? 0xFFFF : bytes);
        text_make_room(p, 0);
        if (p->u.text.buf) {
            p->u.text.buf[p->u.text.len] = '\0';
        }
        term_panel_touch(p);
    }
}

void term_text_autoscroll(term_panel_t *p, bool on) {
    if (is_text(p)) {
        p->u.text.autoscroll = on;
        p->u.text.follow = on;
        term_panel_touch(p);
    }
}

void term_text_scroll(term_panel_t *p, int rows) {
    if (is_text(p)) {
        int top = p->u.text.top + rows;
        p->u.text.top = top < 0 ? 0 : top;
        p->u.text.follow = 0; /* the next render re-follows at the end */
        term_panel_touch(p);
    }
}

/* Word-wraps the text into rows of the panel's width, at spaces; '\n' forces
 * a break and over-long words are split. Returns how many rows it takes. If
 * `draw`, rows top..top+height-1 are drawn in `attr`, aligned. */
static int text_layout(term_panel_t *p, bool draw, int top, uint8_t attr) {
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    if (w <= 0) {
        return 0;
    }
    const char *s = p->u.text.buf ? p->u.text.buf : "";
    char line[TERM_COLS];
    uint8_t line_style[TERM_COLS]; /* inline style of each character in `line` */
    uint8_t style = 0;
    int len = 0;
    int rows = 0;
    bool soft = false; /* this row began with a wrap, not a '\n' or the start */

#define EMIT()                                                                    \
    do {                                                                          \
        if (draw && rows >= top && rows - top < h) {                              \
            int n = len;                                                          \
            while (n > 0 && line[n - 1] == ' ') {                                 \
                n--;                                                              \
            }                                                                     \
            int col = p->align == TERM_ALIGN_CENTER ? (w - n) / 2 : 0;            \
            for (int i = 0; i < n; i++) {                                         \
                term_put(p, col + i, rows - top, (uint8_t)line[i], attr | line_style[i]); \
            }                                                                     \
        }                                                                         \
        rows++;                                                                   \
        len = 0;                                                                  \
    } while (0)

    while (*s) {
        if (*s == TERM_ESC) {
            if (esc_len(s) == 2) {
                style = s[1] & 0x1F;
            }
            s += esc_len(s);
        } else if (*s == '\n') {
            EMIT();
            soft = false;
            style = 0; /* inline styles end with the line */
            s++;
        } else if (*s == ' ' || *s == '\t') {
            /* Indentation is kept, except on a wrapped row; a tab is spaces
             * to the next stop. */
            if (!(len == 0 && soft)) {
                int stop = *s == '\t' ? (len + TERM_TAB) & ~(TERM_TAB - 1) : len + 1;
                while (len < stop && len < w) {
                    line_style[len] = style;
                    line[len++] = ' ';
                }
            }
            s++;
        } else {
            /* A word: `n` bytes, `width` of them shown (escapes take none). */
            int n = 0;
            int width = 0;
            while (s[n] && s[n] != ' ' && s[n] != '\t' && s[n] != '\n') {
                if (s[n] == TERM_ESC) {
                    n += esc_len(s + n);
                } else {
                    n++;
                    width++;
                }
            }
            if (len > 0 && len + width > w) {
                EMIT();
                soft = true;
            }
            for (int i = 0; i < n; i++) {
                if (s[i] == TERM_ESC) {
                    if (esc_len(s + i) == 2) {
                        style = s[i + 1] & 0x1F;
                    }
                    i += esc_len(s + i) - 1;
                    continue;
                }
                if (len >= w) {
                    EMIT();
                    soft = true;
                }
                line_style[len] = style;
                line[len++] = s[i];
            }
            s += n;
        }
    }
    if (len > 0) {
        EMIT();
    }
#undef EMIT
    return rows;
}

static void text_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    for (int r = 0; r < h; r++) {
        fill_row(p, r, 0, w, p->attr);
    }

    int rows = text_layout(p, false, 0, p->attr);
    int max_top = rows > h ? rows - h : 0;
    if (p->u.text.follow || p->u.text.top > max_top) {
        p->u.text.top = max_top;
    }
    if (p->u.text.autoscroll && p->u.text.top == max_top) {
        p->u.text.follow = 1; /* back at the end: keep up with new text */
    }
    int top = p->u.text.top;
    text_layout(p, true, top, p->attr);

    if (w > 0 && h > 0 && rows > h) {
        if (top > 0) {
            term_put(p, w - 1, 0, TERM_CH_ARROW_U, p->attr); /* more above */
        }
        if (top < max_top) {
            term_put(p, w - 1, h - 1, TERM_CH_ARROW_D, p->attr); /* more below */
        }
    }
}

static bool text_key(term_panel_t *p, const term_event_t *ev) {
    if (!p->focusable || (ev->key != TERM_KEY_UP && ev->key != TERM_KEY_DOWN)) {
        return false;
    }
    int max_top = text_layout(p, false, 0, 0) - term_panel_height(p);
    int top = p->u.text.top + (ev->key == TERM_KEY_UP ? -1 : 1);
    if (top > max_top) {
        top = max_top;
    }
    p->u.text.top = top < 0 ? 0 : top;
    p->u.text.follow = 0; /* re-follows at the end if autoscroll is on */
    return true;
}

/* ---- Button -------------------------------------------------------------- */

void term_make_button(term_panel_t *p, const char *label) {
    if (begin_widget(p, TERM_KIND_BUTTON, true)) {
        p->align = TERM_ALIGN_CENTER;
        p->attr = TERM_ATTR_REVERSE;
        p->submit = 1;
        text_init(p, label);
    }
}

static void button_draw(term_panel_t *p) {
    text_draw(p);
    if (focused(p)) {
        term_put(p, 0, 0, TERM_CH_ARROW_R, p->attr);
    }
}

/* ---- Checkbox ------------------------------------------------------------ */

void term_make_checkbox(term_panel_t *p, const char *label, bool checked) {
    if (begin_widget(p, TERM_KIND_CHECKBOX, true)) {
        text_init(p, label);
        p->u.text.checked = checked;
    }
}

bool term_checkbox_checked(const term_panel_t *p) {
    return p->kind == TERM_KIND_CHECKBOX && p->u.text.checked;
}

void term_checkbox_set(term_panel_t *p, bool checked) {
    if (p->kind == TERM_KIND_CHECKBOX) {
        p->u.text.checked = checked;
        term_panel_touch(p);
    }
}

static void checkbox_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    uint8_t attr = focused(p) ? p->focus_attr : p->attr;
    fill_row(p, 0, 0, w, attr);
    term_put(p, 0, 0, '[', attr);
    term_put(p, 1, 0, p->u.text.checked ? 'x' : ' ', attr);
    term_put(p, 2, 0, ']', attr);
    put_str(p, 4, 0, p->u.text.buf ? p->u.text.buf : "", w - 4, attr);
}

static bool checkbox_key(term_panel_t *p, const term_event_t *ev) {
    if (ev->key != TERM_KEY_ENTER) {
        return false;
    }
    p->u.text.checked = !p->u.text.checked;
    term_emit(p->ctx, TERM_EV_CHANGE, p, p->u.text.checked);
    return true;
}

/* ---- List ---------------------------------------------------------------- */

void term_make_list(term_panel_t *p, const char *const *items, int count) {
    if (begin_widget(p, TERM_KIND_LIST, true)) {
        p->u.list.items = items;
        p->u.list.count = count;
    }
}

void term_list_set_items(term_panel_t *p, const char *const *items, int count) {
    if (p->kind != TERM_KIND_LIST) {
        return;
    }
    p->u.list.items = items;
    p->u.list.count = count;
    if (p->u.list.sel >= count) {
        p->u.list.sel = count > 0 ? count - 1 : 0;
    }
    p->u.list.top = 0;
    term_panel_touch(p);
}

int term_list_selected(const term_panel_t *p) {
    if (p->kind != TERM_KIND_LIST || p->u.list.count == 0) {
        return -1;
    }
    return p->u.list.sel;
}

void term_list_select(term_panel_t *p, int index) {
    if (p->kind != TERM_KIND_LIST || p->u.list.count == 0) {
        return;
    }
    if (index < 0) {
        index = 0;
    }
    if (index >= p->u.list.count) {
        index = p->u.list.count - 1;
    }
    p->u.list.sel = index;
    term_panel_touch(p);
}

/* Keeps the selected row inside the visible window. */
static void list_scroll_to_selection(term_panel_t *p) {
    int h = term_panel_height(p);
    int sel = p->u.list.sel;
    int top = p->u.list.top;
    if (h <= 0) {
        return;
    }
    if (sel < top) {
        top = sel;
    } else if (sel >= top + h) {
        top = sel - h + 1;
    }
    p->u.list.top = top;
}

/* Column 0 is a gutter holding the selection arrow. A scrollbar takes the
 * last column when the list is longer than the panel. */
static void list_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    int count = p->u.list.count;
    bool bar = count > h;
    int text_w = w - 1 - (bar ? 1 : 0);

    list_scroll_to_selection(p);

    for (int row = 0; row < h; row++) {
        int i = p->u.list.top + row;
        if (i >= count) {
            break;
        }
        bool selected = (i == p->u.list.sel);
        uint8_t attr = (selected && focused(p)) ? p->focus_attr : p->attr;

        fill_row(p, row, 0, w - (bar ? 1 : 0), attr);
        if (selected) {
            term_put(p, 0, row, TERM_CH_ARROW_R, attr);
        }
        if (text_w > 0) {
            put_str(p, 1, row, p->u.list.items[i], text_w, attr);
        }
    }

    if (bar && h > 0) {
        int thumb = (int)((long)p->u.list.sel * (h - 1) / (count - 1));
        for (int row = 0; row < h; row++) {
            term_put(p, w - 1, row, row == thumb ? TERM_CH_BLOCK : TERM_CH_SHADE, p->attr);
        }
    }
}

static bool list_key(term_panel_t *p, const term_event_t *ev) {
    int count = p->u.list.count;
    switch (ev->key) {
    case TERM_KEY_UP:
        if (count > 0) {
            p->u.list.sel = p->u.list.sel > 0 ? p->u.list.sel - 1 : count - 1;
            term_emit(p->ctx, TERM_EV_CHANGE, p, p->u.list.sel);
        }
        return true;
    case TERM_KEY_DOWN:
        if (count > 0) {
            p->u.list.sel = p->u.list.sel + 1 < count ? p->u.list.sel + 1 : 0;
            term_emit(p->ctx, TERM_EV_CHANGE, p, p->u.list.sel);
        }
        return true;
    case TERM_KEY_ENTER:
        if (count > 0) {
            term_emit(p->ctx, TERM_EV_SUBMIT, p, p->u.list.sel);
        }
        return true;
    default:
        return false;
    }
}

/* ---- Input --------------------------------------------------------------- */

void term_make_input(term_panel_t *p) {
    begin_widget(p, TERM_KIND_INPUT, true);
}

const char *term_input_text(const term_panel_t *p) {
    return p->kind == TERM_KIND_INPUT ? p->u.input.buf : "";
}

void term_input_set(term_panel_t *p, const char *text) {
    if (p->kind != TERM_KIND_INPUT) {
        return;
    }
    size_t n = strlen(text);
    if (n > TERM_INPUT_MAX) {
        n = TERM_INPUT_MAX;
    }
    memcpy(p->u.input.buf, text, n);
    p->u.input.buf[n] = '\0';
    p->u.input.len = n;
    p->u.input.cur = n;
    term_panel_touch(p);
}

static void input_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    if (w <= 0) {
        return;
    }

    /* Scroll horizontally so the cursor stays on screen. */
    int scroll = p->u.input.scroll;
    if (p->u.input.cur < scroll) {
        scroll = p->u.input.cur;
    } else if (p->u.input.cur >= scroll + w) {
        scroll = p->u.input.cur - w + 1;
    }
    p->u.input.scroll = scroll;

    for (int col = 0; col < w; col++) {
        int i = scroll + col;
        uint8_t ch = i < p->u.input.len ? (uint8_t)p->u.input.buf[i] : 0;
        uint8_t attr = (focused(p) && i == p->u.input.cur) ? p->focus_attr : p->attr;
        term_put(p, col, 0, ch, attr);
    }
}

static bool input_key(term_panel_t *p, const term_event_t *ev) {
    switch (ev->key) {
    case TERM_KEY_CHAR:
        if (p->u.input.len < TERM_INPUT_MAX) {
            memmove(&p->u.input.buf[p->u.input.cur + 1], &p->u.input.buf[p->u.input.cur],
                    p->u.input.len - p->u.input.cur + 1);
            p->u.input.buf[p->u.input.cur++] = ev->ch;
            p->u.input.len++;
            term_emit(p->ctx, TERM_EV_CHANGE, p, 0);
        }
        return true;
    case TERM_KEY_DEL:
        if (p->u.input.cur > 0) {
            memmove(&p->u.input.buf[p->u.input.cur - 1], &p->u.input.buf[p->u.input.cur],
                    p->u.input.len - p->u.input.cur + 1);
            p->u.input.cur--;
            p->u.input.len--;
            term_emit(p->ctx, TERM_EV_CHANGE, p, 0);
        }
        return true;
    case TERM_KEY_LEFT:
        if (p->u.input.cur > 0) {
            p->u.input.cur--;
        }
        return true;
    case TERM_KEY_RIGHT:
        if (p->u.input.cur < p->u.input.len) {
            p->u.input.cur++;
        }
        return true;
    case TERM_KEY_CLEAR:
        if (p->u.input.len == 0) {
            return false; /* nothing to clear: let the app have it (e.g. to quit) */
        }
        p->u.input.buf[0] = '\0';
        p->u.input.len = 0;
        p->u.input.cur = 0;
        term_emit(p->ctx, TERM_EV_CHANGE, p, 0);
        return true;
    case TERM_KEY_ENTER:
        term_emit(p->ctx, TERM_EV_SUBMIT, p, 0);
        return true;
    default:
        return false;
    }
}

/* ---- Progress ------------------------------------------------------------ */

void term_make_progress(term_panel_t *p, int max) {
    if (begin_widget(p, TERM_KIND_PROGRESS, false)) {
        p->u.progress.max = max > 0 ? max : 1;
    }
}

void term_progress_set(term_panel_t *p, int value) {
    if (p->kind != TERM_KIND_PROGRESS) {
        return;
    }
    if (value < 0) {
        value = 0;
    }
    if (value > p->u.progress.max) {
        value = p->u.progress.max;
    }
    p->u.progress.value = value;
    term_panel_touch(p);
}

static void progress_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    int filled = (int)((unsigned long)p->u.progress.value * w / p->u.progress.max);
    for (int c = 0; c < w; c++) {
        term_put(p, c, 0, c < filled ? TERM_CH_BLOCK : TERM_CH_SHADE, p->attr);
    }
}

/* ---- Dispatch ------------------------------------------------------------ */

void term_widget_draw(term_panel_t *p) {
    switch (p->kind) {
    case TERM_KIND_TEXT:     text_draw(p);     break;
    case TERM_KIND_BUTTON:   button_draw(p);   break;
    case TERM_KIND_CHECKBOX: checkbox_draw(p); break;
    case TERM_KIND_LIST:     list_draw(p);     break;
    case TERM_KIND_INPUT:    input_draw(p);    break;
    case TERM_KIND_PROGRESS: progress_draw(p); break;
    default:                                   break;
    }
}

bool term_widget_key(term_panel_t *p, const term_event_t *ev) {
    switch (p->kind) {
    case TERM_KIND_TEXT:     return text_key(p, ev);
    case TERM_KIND_CHECKBOX: return checkbox_key(p, ev);
    case TERM_KIND_LIST:     return list_key(p, ev);
    case TERM_KIND_INPUT:    return input_key(p, ev);
    default:                 return false;
    }
}

void term_widget_free(term_panel_t *p) {
    if (is_text(p)) {
        free(p->u.text.buf);
        p->u.text.buf = NULL;
    }
}
