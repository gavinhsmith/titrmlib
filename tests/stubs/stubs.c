/* Host implementations of the CE calls titrmlib uses. Drawing lands in
 * stub_fb, which titrmlib writes into directly as gfx_vbuffer; ASan catches
 * writes outside it. */

#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <ti/getcsc.h>
#include <ti/screen.h>

#include "titrm_internal.h"

uint8_t stub_fb[GFX_LCD_HEIGHT][GFX_LCD_WIDTH];
int stub_gfx_open;
int stub_quit_when_idle = 1;

static const uint8_t *keys;
static int key_count;
static int key_pos;

void gfx_Begin(void) { stub_gfx_open = 1; }
void gfx_End(void) { stub_gfx_open = 0; }

void gfx_FillScreen(uint8_t index) {
    for (int y = 0; y < GFX_LCD_HEIGHT; y++) {
        for (int x = 0; x < GFX_LCD_WIDTH; x++) {
            stub_fb[y][x] = index;
        }
    }
}

void os_ClrHome(void) {}

void stub_keys(const uint8_t *k, int n) {
    keys = k;
    key_count = n;
    key_pos = 0;
}

uint8_t stub_kb_data[8];
static uint8_t held_sk;  /* stub_hold(): kept down across scans */
static int released = 1; /* the last scripted key has been let go */

static void press(uint8_t sk) {
    stub_kb_data[7 - ((sk - 1) >> 3)] |= 1 << ((sk - 1) & 7);
}

void stub_hold(uint8_t sk) { held_sk = sk; }
void stub_release(void) { held_sk = 0; }

/* Each scripted key is down for one scan and up for the next, so every one
 * is a separate press, even the same key twice. */
void kb_Scan(void) {
    memset(stub_kb_data, 0, sizeof stub_kb_data);
    if (held_sk) {
        press(held_sk);
    }
    if (!released) {
        released = 1;
        return;
    }
    if (key_pos < key_count) {
        press(keys[key_pos++]);
        released = 0;
        return;
    }
    /* Idle means the script is used up and the library has handled every key
     * it queued (it also reads the keypad while drawing). */
    if (stub_quit_when_idle && !held_sk && !term_keys_pending()) {
        term_quit(term_init(), STUB_IDLE_RESULT); /* term_init() returns the live context */
    }
}
