/*
 * titrmlib demo: a mock Wi-Fi manager (the sort of screen Tincan needs).
 *
 *   +- title bar: alpha indicator, spinner ------------------------------+
 *   | Networks (list) | Details (custom draw panel)                      |
 *   |                 | Log (scrollback)                                 |
 *   +- Command (input) --------------------------------------------------+
 *   | progress bar | key hints                                           |
 *
 * Keys: up/down pick a network, [enter] connects, [vars] moves focus,
 * [y=] shows/hides the details panel, [window] clears the log, [clear] quits.
 * Type "help" in the command box for text commands (use [alpha] for letters).
 */

#include <string.h>

#include "titrm.h"

typedef struct {
    const char *label; /* list text, starts with a signal-strength glyph */
    const char *ssid;
    int signal; /* 0..3 */
    const char *security;
} network_t;

static const network_t networks[] = {
    {TERM_S_SIG3 "HomeWiFi",      "HomeWiFi",      3, "WPA2"},
    {TERM_S_SIG3 "IoT-Lab",       "IoT-Lab",       3, "WPA3"},
    {TERM_S_SIG2 "CoffeeShop",    "CoffeeShop",    2, "Open"},
    {TERM_S_SIG2 "xfinitywifi",   "xfinitywifi",   2, "WPA2"},
    {TERM_S_SIG1 "Library-Guest", "Library-Guest", 1, "Open"},
    {TERM_S_SIG1 "Neighbor5G",    "Neighbor5G",    1, "WPA2"},
    {TERM_S_SIG0 "FBI Van #4",    "FBI Van #4",    0, "WPA2"},
};
#define NUM_NETWORKS ((int)(sizeof networks / sizeof networks[0]))

#define CONNECT_STEPS 20

typedef struct {
    term_panel_t *title;
    term_panel_t *list;
    term_panel_t *details;
    term_panel_t *log;
    term_panel_t *input;
    term_panel_t *progress;

    const char *items[NUM_NETWORKS];
    int connecting; /* index being connected to, or -1 */
    int connected;  /* index of the current connection, or -1 */
    int step;
    unsigned ticks;
} demo_t;

/* ---- Draw callbacks ------------------------------------------------------ */

static void draw_title(term_ctx_t *ctx, term_panel_t *p, void *user) {
    demo_t *d = user;
    static const char spinner[] = "|/-\\";

    term_panel_set_attr(p, TERM_ATTR_REVERSE);
    term_panel_repeat(p, ' ', term_panel_width(p));

    term_panel_move(p, 1, 0);
    term_panel_print(p, "titrmlib demo");

    if (d->connecting >= 0) {
        term_panel_move(p, term_panel_width(p) - 12, 0);
        term_panel_putc(p, spinner[(d->ticks / 2) % 4]);
    }

    int alpha = term_alpha_mode(ctx);
    if (alpha) {
        term_panel_move(p, term_panel_width(p) - 7, 0);
        term_panel_print(p, alpha == 2 ? "A-LOCK" : "ALPHA");
    }
}

static void draw_details(term_ctx_t *ctx, term_panel_t *p, void *user) {
    (void)ctx;
    demo_t *d = user;
    const network_t *n = &networks[term_list_selected(d->list)];

    term_panel_printf(p, "SSID:     %s\n", n->ssid);
    term_panel_print(p, "Signal:   ");
    term_panel_putc(p, TERM_CH_SIG0 + n->signal);
    term_panel_printf(p, "\nSecurity: %s\n", n->security);

    term_panel_print(p, "Status:   ");
    int i = term_list_selected(d->list);
    if (i == d->connected) {
        term_panel_putc(p, TERM_CH_CHECK);
        term_panel_print(p, " connected");
    } else if (i == d->connecting) {
        term_panel_putc(p, TERM_CH_DOT);
        term_panel_print(p, " connecting");
    } else {
        term_panel_putc(p, TERM_CH_DOT_EMPTY);
        term_panel_print(p, " idle");
    }
}

/* ---- Logic --------------------------------------------------------------- */

static bool equals_nocase(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        char x = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
        char y = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
        if (x != y) {
            return false;
        }
    }
    return *a == *b;
}

static void toggle_details(demo_t *d) {
    /* Hidden panels take no space: the log grows into the gap next frame. */
    term_panel_show(d->details, !term_panel_visible(d->details));
}

static void connect_to(demo_t *d, int index) {
    if (d->connecting >= 0) {
        term_log_print(d->log, "Busy: already connecting");
        return;
    }
    d->connecting = index;
    d->step = 0;
    term_progress_set(d->progress, 0);
    term_log_printf(d->log, "Connecting: %s", networks[index].ssid);
}

static void run_command(term_ctx_t *ctx, demo_t *d, const char *cmd) {
    term_log_printf(d->log, "> %s", cmd);
    if (equals_nocase(cmd, "help")) {
        term_log_print(d->log, "help clear info quit");
    } else if (equals_nocase(cmd, "clear")) {
        term_log_clear(d->log);
    } else if (equals_nocase(cmd, "info")) {
        toggle_details(d);
    } else if (equals_nocase(cmd, "quit")) {
        term_quit(ctx, 0);
    } else if (*cmd) {
        term_log_print(d->log, "?? try: help");
    }
}

static void on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    demo_t *d = state;

    switch (ev->type) {
    case TERM_EV_START:
        term_log_print(d->log, "Welcome to titrmlib!");
        term_log_print(d->log, "[enter] to connect");
        break;

    case TERM_EV_TICK:
        d->ticks++;
        if (d->connecting >= 0) {
            d->step++;
            term_progress_set(d->progress, d->step * 100 / CONNECT_STEPS);
            if (d->step >= CONNECT_STEPS) {
                d->connected = d->connecting;
                d->connecting = -1;
                term_log_printf(d->log, TERM_S_CHECK "Connected: %s", networks[d->connected].ssid);
            }
        }
        break;

    case TERM_EV_SELECT:
        if (ev->panel == d->list) {
            connect_to(d, ev->value);
        }
        break;

    case TERM_EV_SUBMIT:
        run_command(ctx, d, term_input_text(ev->panel));
        term_input_set(ev->panel, "");
        break;

    case TERM_EV_KEY:
        if (ev->key == TERM_KEY_F1) {
            toggle_details(d);
        } else if (ev->key == TERM_KEY_F2) {
            term_log_clear(d->log);
        } else if (ev->key == TERM_KEY_CLEAR) {
            term_quit(ctx, 0);
        }
        break;
    }
}

/* ---- Setup --------------------------------------------------------------- */

int main(void) {
    static demo_t d;
    d.connecting = -1;
    d.connected = -1;
    for (int i = 0; i < NUM_NETWORKS; i++) {
        d.items[i] = networks[i].label;
    }

    term_ctx_t *ctx = term_init();
    term_panel_t *root = term_root(ctx);

    /* Screen: title bar / body / command line / status bar, stacked. */
    d.title = term_split(root, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_t *body = term_split(root, TERM_VERTICAL, TERM_FILL);
    d.input = term_split(root, TERM_VERTICAL, TERM_FIXED(3));
    term_panel_t *status = term_split(root, TERM_VERTICAL, TERM_FIXED(1));

    /* Body: network list on the left, details over log on the right. */
    d.list = term_split(body, TERM_HORIZONTAL, TERM_PERCENT(40));
    term_panel_t *right = term_split(body, TERM_HORIZONTAL, TERM_FILL);
    d.details = term_split(right, TERM_VERTICAL, TERM_FIXED(6));
    d.log = term_split(right, TERM_VERTICAL, TERM_FILL);

    /* Status bar: progress bar and key hints, side by side. */
    d.progress = term_split(status, TERM_HORIZONTAL, TERM_FIXED(16));
    term_panel_t *hints = term_split(status, TERM_HORIZONTAL, TERM_FILL);

    term_panel_set_draw(d.title, draw_title, &d);

    term_make_list(d.list, d.items, NUM_NETWORKS);
    term_panel_set_border(d.list, true);
    term_panel_set_title(d.list, "Networks");

    term_panel_set_border(d.details, true);
    term_panel_set_title(d.details, "Details");
    term_panel_set_draw(d.details, draw_details, &d);

    term_make_log(d.log, 32);
    term_panel_set_border(d.log, true);
    term_panel_set_title(d.log, "Log");

    term_make_input(d.input);
    term_panel_set_border(d.input, true);
    term_panel_set_title(d.input, "Command");

    term_make_progress(d.progress, 100);
    term_make_text(hints, " [vars]focus [y=]info [clear]quit");

    term_set_tick(ctx, 100);
    term_focus(ctx, d.list);

    term_run(ctx, on_event, &d);
    term_shutdown(ctx);
    return 0;
}
