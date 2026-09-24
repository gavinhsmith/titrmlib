/*
 * titrmlib demo: a mock Wi-Fi manager (the sort of screen Tincan needs).
 *
 *   +- title bar: alpha indicator, spinner ------------------------------+
 *   | Networks (list) | Details (text)                                   |
 *   |                 | Log (text that follows its end)                  |
 *   +- Command (input) --------------------------------------------------+
 *   | progress bar | key hints                                           |
 *
 * Keys: up/down pick a network, [enter] connects (secured networks ask for a
 * password in a dialog), [vars] moves focus, [y=] shows/hides the details,
 * [window] clears the log, [mode] shows the help scene, [clear] quits.
 * Type "help" in the command box for text commands (use [alpha] for letters).
 */

#include <stdio.h>
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
    term_panel_t *main;
    term_panel_t *help;

    term_panel_t *title;
    term_panel_t *list;
    term_panel_t *details;
    term_panel_t *log;
    term_panel_t *input;
    term_panel_t *progress;

    /* The password dialog, while it's open. */
    term_panel_t *dialog;
    term_panel_t *password;
    term_panel_t *connect_button;
    term_panel_t *cancel_button;
    char dialog_title[32];
    int dialog_network;

    const char *items[NUM_NETWORKS];
    int connecting; /* index being connected to, or -1 */
    int connected;  /* index of the current connection, or -1 */
    int step;
    unsigned ticks;
} demo_t;

/* ---- Output -------------------------------------------------------------- */

/* Panel output is retained, so these rewrite a panel whenever what it shows
 * may have changed (on_event calls them after every event). */

static void show_title(term_ctx_t *ctx, demo_t *d) {
    static const char spinner[] = "|/-\\";
    term_panel_t *p = d->title;

    term_panel_clear(p);

    term_panel_move(p, 1, 0);
    term_panel_print(p, TERM_S_BOLD "titrmlib" TERM_S_NORMAL " demo");

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

static void show_details(demo_t *d) {
    const network_t *n = &networks[term_list_selected(d->list)];
    int i = term_list_selected(d->list);
    char text[160];

    /* Inline styles: the SSID in bold, the status styled by state. */
    snprintf(text, sizeof text,
             "SSID:     " TERM_S_BOLD "%s\nSignal:   %c\nSecurity: %s\nStatus:   %s", n->ssid,
             TERM_CH_SIG0 + n->signal, n->security,
             i == d->connected    ? TERM_S_BOLD "connected"
             : i == d->connecting ? TERM_S_ITALIC "connecting..."
                                  : "idle");
    term_text_set(d->details, text);
}

/* ---- Connecting ---------------------------------------------------------- */

static void start_connecting(demo_t *d, int index) {
    d->connecting = index;
    d->step = 0;
    term_progress_set(d->progress, 0);
    term_text_appendf(d->log, TERM_S_ITALIC "Connecting:" TERM_S_NORMAL " %s\n", networks[index].ssid);
}

/* Secured networks ask for a password first, in a dialog over the screen. */
static void open_password_dialog(term_ctx_t *ctx, demo_t *d, int index) {
    d->dialog_network = index;
    snprintf(d->dialog_title, sizeof d->dialog_title, "Password: %s", networks[index].ssid);

    d->dialog = term_overlay_open_centered(ctx, 32, 6);
    term_panel_set_colors(d->dialog, TERM_COLOR_WHITE, TERM_COLOR_BLUE); /* its panels inherit */
    term_panel_set_border(d->dialog, true);
    term_panel_set_title(d->dialog, d->dialog_title);
    d->password = term_split(d->dialog, TERM_VERTICAL, TERM_FIXED(1));
    term_make_input(d->password);
    term_split(d->dialog, TERM_VERTICAL, TERM_FIXED(1)); /* gap */
    term_panel_t *buttons = term_split(d->dialog, TERM_VERTICAL, TERM_FIXED(1));
    d->connect_button = term_split(buttons, TERM_HORIZONTAL, TERM_FILL);
    term_split(buttons, TERM_HORIZONTAL, TERM_FIXED(2)); /* gap */
    d->cancel_button = term_split(buttons, TERM_HORIZONTAL, TERM_FILL);
    term_make_button(d->connect_button, "Connect");
    term_make_button(d->cancel_button, "Cancel");

    term_focus(ctx, d->password); /* focus returns to the list when it closes */
}

static void close_dialog(demo_t *d) {
    term_overlay_close(d->dialog);
    d->dialog = NULL;
}

static void connect_to(term_ctx_t *ctx, demo_t *d, int index) {
    if (d->connecting >= 0) {
        term_text_append(d->log, "Busy: already connecting\n");
    } else if (strcmp(networks[index].security, "Open") != 0) {
        open_password_dialog(ctx, d, index);
    } else {
        start_connecting(d, index);
    }
}

/* ---- Commands ------------------------------------------------------------ */

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
    /* Hidden panels take no space: the log grows into the gap. */
    term_panel_show(d->details, !term_panel_visible(d->details));
}

static void run_command(term_ctx_t *ctx, demo_t *d, const char *cmd) {
    term_text_appendf(d->log, "> %s\n", cmd);
    if (equals_nocase(cmd, "help")) {
        term_text_append(d->log, "help clear info quit\n");
    } else if (equals_nocase(cmd, "clear")) {
        term_text_clear(d->log);
    } else if (equals_nocase(cmd, "info")) {
        toggle_details(d);
    } else if (equals_nocase(cmd, "quit")) {
        term_quit(ctx, 0);
    } else if (*cmd) {
        term_text_append(d->log, "?? try: " TERM_S_BOLD "help\n");
    }
}

/* ---- Focus --------------------------------------------------------------- */

/* [vars] cycles focus through `order`, starting over after the last. */
static void focus_next(term_ctx_t *ctx, term_panel_t *const *order, int n) {
    term_panel_t *now = term_focused(ctx);
    int next = 0;
    for (int i = 0; i < n; i++) {
        if (order[i] == now) {
            next = (i + 1) % n;
        }
    }
    term_focus(ctx, order[next]);
}

/* ---- Events -------------------------------------------------------------- */

/* The password dialog's events, while it's open. */
static bool dialog_event(term_ctx_t *ctx, demo_t *d, const term_event_t *ev) {
    if (ev->type == TERM_EV_SUBMIT && (ev->panel == d->password || ev->panel == d->connect_button)) {
        term_text_appendf(d->log, "Password: %d characters\n", (int)strlen(term_input_text(d->password)));
        int index = d->dialog_network;
        close_dialog(d);
        start_connecting(d, index);
        return true;
    }
    if ((ev->type == TERM_EV_SUBMIT && ev->panel == d->cancel_button) ||
        (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_CLEAR)) {
        close_dialog(d);
        return true;
    }
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_VARS) {
        term_panel_t *order[] = {d->password, d->connect_button, d->cancel_button};
        focus_next(ctx, order, 3);
        return true;
    }
    /* Arrows the focused widget doesn't use move between the password (it
     * keeps left/right for its cursor) and the buttons below it. */
    term_panel_t *now = term_focused(ctx);
    bool on_button = now == d->connect_button || now == d->cancel_button;
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_DOWN && now == d->password) {
        term_focus(ctx, d->connect_button);
        return true;
    }
    if (ev->type == TERM_EV_KEY && ev->key == TERM_KEY_UP && on_button) {
        term_focus(ctx, d->password);
        return true;
    }
    if (ev->type == TERM_EV_KEY && (ev->key == TERM_KEY_LEFT || ev->key == TERM_KEY_RIGHT) && on_button) {
        term_focus(ctx, now == d->connect_button ? d->cancel_button : d->connect_button);
        return true;
    }
    return false;
}

static bool on_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    demo_t *d = state;

    if (d->dialog && dialog_event(ctx, d, ev)) {
        /* handled */
    } else {
        switch (ev->type) {
        case TERM_EV_START:
            term_text_append(d->log, "Welcome to " TERM_S_BOLD "titrmlib" TERM_S_NORMAL "!\n");
            term_text_append(d->log, TERM_S_BOLD "[enter]" TERM_S_NORMAL " to connect\n");
            break;

        case TERM_EV_TICK:
            d->ticks++;
            if (d->connecting >= 0) {
                d->step++;
                term_progress_set(d->progress, d->step * 100 / CONNECT_STEPS);
                if (d->step >= CONNECT_STEPS) {
                    d->connected = d->connecting;
                    d->connecting = -1;
                    term_text_appendf(d->log, "Connected to " TERM_S_BOLD "%s\n", networks[d->connected].ssid);
                }
            }
            break;

        case TERM_EV_SUBMIT:
            if (ev->panel == d->list) {
                connect_to(ctx, d, ev->value);
            } else if (ev->panel == d->input) {
                run_command(ctx, d, term_input_text(d->input));
                term_input_set(d->input, "");
            }
            break;

        case TERM_EV_KEY:
            if (ev->key == TERM_KEY_VARS) {
                term_panel_t *order[] = {d->list, d->input, d->log};
                focus_next(ctx, order, 3);
            } else if (ev->key == TERM_KEY_F1) {
                toggle_details(d);
            } else if (ev->key == TERM_KEY_F2) {
                term_text_clear(d->log);
            } else if (ev->key == TERM_KEY_MODE) {
                term_scene_switch(ctx, d->help);
            } else if (ev->key == TERM_KEY_CLEAR) {
                term_quit(ctx, 0);
            }
            break;

        default: /* TERM_EV_CHANGE (the selection moved), TERM_EV_FOCUS_LOST */
            break;
        }
    }

    show_title(ctx, d);
    show_details(d);
    return true;
}

/* The help scene's own handler: any of its keys goes back. */
static bool help_event(term_ctx_t *ctx, const term_event_t *ev, void *state) {
    demo_t *d = state;
    if (ev->type == TERM_EV_KEY && (ev->key == TERM_KEY_MODE || ev->key == TERM_KEY_CLEAR)) {
        term_scene_switch(ctx, d->main);
        term_focus(ctx, d->list);
        return true;
    }
    return ev->type == TERM_EV_KEY; /* other keys do nothing here */
}

/* ---- Setup --------------------------------------------------------------- */

static void build_help(demo_t *d) {
    term_panel_t *box = term_split(d->help, TERM_VERTICAL, TERM_FILL);
    term_panel_set_border(box, true);
    term_panel_set_title(box, "Help");
    /* Headings underlined, keys in bold; a tab lines up the descriptions. */
#define KEY(k) "  " TERM_S_BOLD k TERM_S_NORMAL "\t"
    term_make_text(box,
                   TERM_S_UNDERLINE "Keys\n"
                   KEY("up/down") "pick a network\n"
                   KEY("[enter]") "connect (password if secured)\n"
                   KEY("[vars]") "move focus (arrows too, in the dialog)\n"
                   KEY("[y=]") "\tshow or hide the details\n"
                   KEY("[window]") "clear the log\n"
                   KEY("[alpha]") "type letters in the command box\n"
                   KEY("[mode]") "this help\n"
                   KEY("[clear]") "quit\n"
                   "\n"
                   TERM_S_UNDERLINE "Commands" TERM_S_NORMAL ": help, clear, info, quit\n"
                   "\n"
                   TERM_S_BOLD "[mode]" TERM_S_NORMAL " or " TERM_S_BOLD "[clear]" TERM_S_NORMAL " goes back.");
#undef KEY
}

int main(void) {
    static demo_t d;
    d.connecting = -1;
    d.connected = -1;
    for (int i = 0; i < NUM_NETWORKS; i++) {
        d.items[i] = networks[i].label;
    }

    term_ctx_t *ctx = term_init();
    d.main = term_root(ctx);
    d.help = term_scene_new(ctx, help_event, &d);
    build_help(&d);

    /* Screen: title bar / body / command line / status bar, stacked. */
    d.title = term_split(d.main, TERM_VERTICAL, TERM_FIXED(1));
    term_panel_set_colors(d.title, TERM_COLOR_WHITE, TERM_COLOR_BLUE);
    term_panel_t *body = term_split(d.main, TERM_VERTICAL, TERM_FILL);
    d.input = term_split(d.main, TERM_VERTICAL, TERM_FIXED(3));
    term_panel_t *status = term_split(d.main, TERM_VERTICAL, TERM_FIXED(1));

    /* Body: network list on the left, details over log on the right. */
    d.list = term_split(body, TERM_HORIZONTAL, TERM_PERCENT(40));
    term_panel_t *right = term_split(body, TERM_HORIZONTAL, TERM_FILL);
    d.details = term_split(right, TERM_VERTICAL, TERM_FIXED(6));
    d.log = term_split(right, TERM_VERTICAL, TERM_FILL);

    /* Status bar: progress bar and key hints, side by side. */
    d.progress = term_split(status, TERM_HORIZONTAL, TERM_FIXED(16));
    term_panel_t *hints = term_split(status, TERM_HORIZONTAL, TERM_FILL);

    term_make_list(d.list, d.items, NUM_NETWORKS);
    term_panel_set_border(d.list, true);
    term_panel_set_title(d.list, "Networks");

    term_make_text(d.details, "");
    term_panel_set_border(d.details, true);
    term_panel_set_title(d.details, "Details");

    term_make_text(d.log, "");
    term_text_autoscroll(d.log, true);
    term_panel_set_focusable(d.log, true); /* up/down scroll it */
    term_panel_set_border(d.log, true);
    term_panel_set_title(d.log, "Log");

    term_make_input(d.input);
    term_panel_set_border(d.input, true);
    term_panel_set_title(d.input, "Command");

    term_make_progress(d.progress, 100);
    term_make_text(hints, " " TERM_S_BOLD "[vars]" TERM_S_NORMAL "focus " TERM_S_BOLD "[y=]" TERM_S_NORMAL
                          "info " TERM_S_BOLD "[mode]" TERM_S_NORMAL "help");

    term_set_tick(ctx, 100);
    term_focus(ctx, d.list);

    term_run(ctx, on_event, &d);
    term_shutdown(ctx);
    return 0;
}
