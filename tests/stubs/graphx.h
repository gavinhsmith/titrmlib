/* Host stand-in for CEdev's graphx.h: just the calls titrmlib makes, drawing
 * into an in-memory 8bpp framebuffer the tests can inspect (stubs.c). */
#ifndef GRAPHX_H
#define GRAPHX_H

#include <stdint.h>

#define GFX_LCD_WIDTH  320
#define GFX_LCD_HEIGHT 240

extern uint8_t stub_fb[GFX_LCD_HEIGHT][GFX_LCD_WIDTH];
extern int stub_gfx_open;

void gfx_Begin(void);
void gfx_End(void);
uint8_t gfx_SetColor(uint8_t index);
void gfx_FillScreen(uint8_t index);
void gfx_HorizLine_NoClip(unsigned int x, uint8_t y, unsigned int length);
void gfx_FillRectangle_NoClip(unsigned int x, uint8_t y, unsigned int width, uint8_t height);

#endif
