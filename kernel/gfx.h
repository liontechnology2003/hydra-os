#ifndef KERNEL_GFX_H
#define KERNEL_GFX_H

#include "types.h"

/* Color packing (ABGR format for 32bpp) */
#define GFX_COLOR(r, g, b) ((uint32)(((b) << 16) | ((g) << 8) | (r)))

/* Common colors */
#define GFX_BLACK       GFX_COLOR(0, 0, 0)
#define GFX_WHITE       GFX_COLOR(255, 255, 255)
#define GFX_RED         GFX_COLOR(255, 0, 0)
#define GFX_GREEN       GFX_COLOR(0, 255, 0)
#define GFX_BLUE        GFX_COLOR(0, 0, 255)
#define GFX_DARK_RED    GFX_COLOR(170, 35, 35)
#define GFX_DARK_BLUE   GFX_COLOR(35, 50, 100)
#define GFX_DARK_GRAY   GFX_COLOR(50, 50, 50)
#define GFX_LIGHT_GRAY  GFX_COLOR(200, 200, 200)
#define GFX_ORANGE      GFX_COLOR(255, 165, 0)

/* Initialize graphics (must be called after vbe_init) */
void gfx_init(void);

/* Pixel operations */
void gfx_put_pixel(uint32 x, uint32 y, uint32 color);
uint32 gfx_get_pixel(uint32 x, uint32 y);

/* Primitive drawing */
void gfx_fill_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint32 color);
void gfx_draw_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint32 color);
void gfx_draw_hline(uint32 x, uint32 y, uint32 w, uint32 color);
void gfx_draw_vline(uint32 x, uint32 y, uint32 h, uint32 color);

/* Clear screen */
void gfx_clear(uint32 color);

/* Text rendering (8x16 bitmap font) */
void gfx_put_char(uint32 x, uint32 y, char c, uint32 fg, uint32 bg, uint32 scale);
void gfx_puts(uint32 x, uint32 y, const char *str, uint32 fg, uint32 bg, uint32 scale);

/* Cursor for text output */
void gfx_set_cursor(uint32 x, uint32 y);
void gfx_text_putchar(char c);
void gfx_text_puts(const char *str);
void gfx_text_set_color(uint32 fg, uint32 bg);
void gfx_scroll_text(void);

/* Screen info */
uint32 gfx_width(void);
uint32 gfx_height(void);
uint32 gfx_pitch(void);
uint32 *gfx_get_framebuffer(void);

/* Damage tracking for compositor */
typedef struct {
    uint32 x, y, w, h;
} gfx_rect_t;

void gfx_set_damage(uint32 x, uint32 y, uint32 w, uint32 h);
int  gfx_has_damage(void);
void gfx_get_damage(gfx_rect_t *rect);
void gfx_clear_damage(void);

#endif /* KERNEL_GFX_H */
