/* Host implementations of the CE calls titrmlib uses. Drawing lands in
 * stub_fb, which titrmlib writes into directly as gfx_vbuffer; ASan catches
 * writes outside it. */

#include <graphx.h>
#include <ti/getcsc.h>
#include <ti/screen.h>

#include "titrm.h"

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

uint8_t os_GetCSC(void) {
    if (key_pos < key_count) {
        return keys[key_pos++];
    }
    if (stub_quit_when_idle) {
        term_quit(term_init(), STUB_IDLE_RESULT); /* term_init() returns the live context */
    }
    return 0;
}
