/* Host implementations of the CE calls titrmlib uses. Drawing lands in
 * stub_fb, and every call is bounds-checked against the 320x240 screen. */

#include <graphx.h>
#include <ti/getcsc.h>
#include <ti/screen.h>

#include <stdio.h>
#include <stdlib.h>

#include "titrm.h"

uint8_t stub_fb[GFX_LCD_HEIGHT][GFX_LCD_WIDTH];
int stub_gfx_open;
int stub_quit_when_idle = 1;

static uint8_t color;
static const uint8_t *keys;
static int key_count;
static int key_pos;

static void off_screen(const char *fn, unsigned x, unsigned y, unsigned w, unsigned h) {
    fprintf(stderr, "%s(%u, %u, %u, %u) draws outside the screen\n", fn, x, y, w, h);
    abort();
}

void gfx_Begin(void) { stub_gfx_open = 1; }
void gfx_End(void) { stub_gfx_open = 0; }

uint8_t gfx_SetColor(uint8_t index) {
    uint8_t old = color;
    color = index;
    return old;
}

void gfx_FillScreen(uint8_t index) {
    for (int y = 0; y < GFX_LCD_HEIGHT; y++) {
        for (int x = 0; x < GFX_LCD_WIDTH; x++) {
            stub_fb[y][x] = index;
        }
    }
}

void gfx_HorizLine_NoClip(unsigned int x, uint8_t y, unsigned int length) {
    if (x + length > GFX_LCD_WIDTH || y >= GFX_LCD_HEIGHT) {
        off_screen("gfx_HorizLine_NoClip", x, y, length, 1);
    }
    for (unsigned int i = 0; i < length; i++) {
        stub_fb[y][x + i] = color;
    }
}

void gfx_FillRectangle_NoClip(unsigned int x, uint8_t y, unsigned int width, uint8_t height) {
    if (x + width > GFX_LCD_WIDTH || (unsigned)y + height > GFX_LCD_HEIGHT) {
        off_screen("gfx_FillRectangle_NoClip", x, y, width, height);
    }
    for (unsigned int r = 0; r < height; r++) {
        for (unsigned int c = 0; c < width; c++) {
            stub_fb[y + r][x + c] = color;
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
