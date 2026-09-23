/* Widgets: panels with built-in content and key handling. Each one draws
 * itself into its panel's retained cells through the same clipped term_put()
 * the app's output uses. Changing a widget's state calls term_panel_touch(),
 * and the framework redraws it before the next frame. */

#include "titrm_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_str(term_panel_t *p, int col, int row, const char *s, int max, uint8_t attr) {
    for (int i = 0; s[i] && i < max; i++) {
        term_put(p, col + i, row, (uint8_t)s[i], attr);
    }
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

/* ---- Text ---------------------------------------------------------------- */

void term_make_text(term_panel_t *p, const char *text) {
    if (begin_widget(p, TERM_KIND_TEXT, false)) {
        p->u.text.text = text;
    }
}

void term_text_set(term_panel_t *p, const char *text) {
    if (p->kind == TERM_KIND_TEXT) {
        p->u.text.text = text;
        term_panel_touch(p);
    }
}

/* Word-wraps at spaces; '\n' forces a break; over-long words are split. */
static void text_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    const char *s = p->u.text.text;
    int row = 0;
    int col = 0;
    bool soft = false; /* this line began with a wrap, not a '\n' or the start */

    while (s && *s && row < h) {
        if (*s == '\n') {
            row++;
            col = 0;
            soft = false;
            s++;
        } else if (*s == ' ') {
            if (col < w && !(col == 0 && soft)) {
                col++; /* indentation is kept, except at the start of a wrapped line */
            }
            s++;
        } else {
            int len = 0;
            while (s[len] && s[len] != ' ' && s[len] != '\n') {
                len++;
            }
            if (col > 0 && col + len > w) {
                row++;
                col = 0;
                soft = true;
            }
            for (int i = 0; i < len && row < h; i++) {
                if (col >= w) {
                    row++;
                    col = 0;
                    soft = true;
                    if (row >= h) {
                        break;
                    }
                }
                term_put(p, col++, row, (uint8_t)s[i], TERM_ATTR_NORMAL);
            }
            s += len;
        }
    }
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
    bool focused = (p == p->ctx->focus);
    bool bar = count > h;
    int text_w = w - 1 - (bar ? 1 : 0);

    list_scroll_to_selection(p);

    for (int row = 0; row < h; row++) {
        int i = p->u.list.top + row;
        if (i >= count) {
            break;
        }
        bool selected = (i == p->u.list.sel);
        uint8_t attr = (selected && focused) ? TERM_ATTR_REVERSE : TERM_ATTR_NORMAL;

        if (selected && focused) {
            for (int c = 0; c < w - (bar ? 1 : 0); c++) {
                term_put(p, c, row, 0, attr);
            }
        }
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
            term_put(p, w - 1, row, row == thumb ? TERM_CH_BLOCK : TERM_CH_SHADE, TERM_ATTR_NORMAL);
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
    bool focused = (p == p->ctx->focus);
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
        uint8_t attr = (focused && i == p->u.input.cur) ? TERM_ATTR_REVERSE : TERM_ATTR_NORMAL;
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

/* ---- Log (scrollback) ---------------------------------------------------- */

void term_make_log(term_panel_t *p, int max_lines) {
    if (max_lines < 1) {
        max_lines = 1;
    }
    if (!begin_widget(p, TERM_KIND_LOG, true)) {
        return;
    }
    p->u.log.lines = calloc(max_lines, TERM_LOG_LINE);
    p->u.log.cap = p->u.log.lines ? max_lines : 0;
}

static char *log_slot(term_panel_t *p, int index) {
    return p->u.log.lines + (size_t)index * TERM_LOG_LINE;
}

static void log_add_line(term_panel_t *p, const char *s, int len) {
    if (p->u.log.cap == 0) {
        return;
    }
    if (len > TERM_LOG_LINE - 1) {
        len = TERM_LOG_LINE - 1;
    }
    char *slot = log_slot(p, p->u.log.head);
    memcpy(slot, s, len);
    slot[len] = '\0';

    p->u.log.head = (p->u.log.head + 1) % p->u.log.cap;
    if (p->u.log.count < p->u.log.cap) {
        p->u.log.count++;
    }
    if (p->u.log.back > 0 && p->u.log.back < p->u.log.count - 1) {
        p->u.log.back++; /* keep the lines being read where they are */
    }
    term_panel_touch(p);
}

void term_log_print(term_panel_t *p, const char *str) {
    if (p->kind != TERM_KIND_LOG) {
        return;
    }
    /* One log line per '\n'. Anything longer than a line is split. */
    while (*str) {
        int len = 0;
        while (str[len] && str[len] != '\n' && len < TERM_LOG_LINE - 1) {
            len++;
        }
        log_add_line(p, str, len);
        str += len;
        if (*str == '\n') {
            str++;
        }
    }
}

void term_log_printf(term_panel_t *p, const char *fmt, ...) {
    char buf[TERM_COLS * 2 + 1];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    term_log_print(p, buf);
}

void term_log_clear(term_panel_t *p) {
    if (p->kind == TERM_KIND_LOG) {
        p->u.log.head = 0;
        p->u.log.count = 0;
        p->u.log.back = 0;
        term_panel_touch(p);
    }
}

static void log_draw(term_panel_t *p) {
    int w = term_panel_width(p);
    int h = term_panel_height(p);
    int count = p->u.log.count;

    /* Index (0 = oldest) of the line shown on the last row. */
    int last = count - 1 - p->u.log.back;
    for (int row = h - 1; row >= 0; row--) {
        int i = last - (h - 1 - row);
        if (i < 0) {
            break;
        }
        const char *line = log_slot(p, (p->u.log.head + p->u.log.cap - count + i) % p->u.log.cap);
        put_str(p, 0, row, line, w, TERM_ATTR_NORMAL);
    }

    if (h > 0 && w > 0) {
        if (last - (h - 1) > 0) {
            term_put(p, w - 1, 0, TERM_CH_ARROW_U, TERM_ATTR_NORMAL); /* older lines above */
        }
        if (p->u.log.back > 0) {
            term_put(p, w - 1, h - 1, TERM_CH_ARROW_D, TERM_ATTR_NORMAL); /* newer lines below */
        }
    }
}

static bool log_key(term_panel_t *p, const term_event_t *ev) {
    int max_back = p->u.log.count - term_panel_height(p);
    if (max_back < 0) {
        max_back = 0;
    }
    switch (ev->key) {
    case TERM_KEY_UP:
        if (p->u.log.back < max_back) {
            p->u.log.back++;
        }
        return true;
    case TERM_KEY_DOWN:
        if (p->u.log.back > 0) {
            p->u.log.back--;
        }
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
        term_put(p, c, 0, c < filled ? TERM_CH_BLOCK : TERM_CH_SHADE, TERM_ATTR_NORMAL);
    }
}

/* ---- Dispatch ------------------------------------------------------------ */

void term_widget_draw(term_panel_t *p) {
    switch (p->kind) {
    case TERM_KIND_TEXT:     text_draw(p);     break;
    case TERM_KIND_LIST:     list_draw(p);     break;
    case TERM_KIND_INPUT:    input_draw(p);    break;
    case TERM_KIND_LOG:      log_draw(p);      break;
    case TERM_KIND_PROGRESS: progress_draw(p); break;
    default:                                   break;
    }
}

bool term_widget_key(term_panel_t *p, const term_event_t *ev) {
    switch (p->kind) {
    case TERM_KIND_LIST:  return list_key(p, ev);
    case TERM_KIND_INPUT: return input_key(p, ev);
    case TERM_KIND_LOG:   return log_key(p, ev);
    default:              return false;
    }
}

void term_widget_free(term_panel_t *p) {
    if (p->kind == TERM_KIND_LOG) {
        free(p->u.log.lines);
        p->u.log.lines = NULL;
    }
}
