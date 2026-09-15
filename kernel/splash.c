#include "splash.h"
#include "gfx.h"
#include "pit.h"

/* Splash colors are stored as 0x00RRGGBB to match the WM framebuffer
 * layout (the GFX_COLOR macro packs BGR instead). */
#define RGB(r, g, b) ((uint32)(((r) << 16) | ((g) << 8) | (b)))

#define SPLASH_BG       RGB(30, 30, 46)
#define SPLASH_TRACK    RGB(72, 70, 92)
#define SPLASH_FILL     RGB(230, 90, 90)
#define SPLASH_BORDER   RGB(200, 200, 210)
#define SPLASH_TEXT     RGB(255, 255, 255)
#define SPLASH_SUB      RGB(200, 200, 200)
#define SPLASH_DIM      RGB(90, 90, 110)

#define BAR_X   212
#define BAR_Y   430
#define BAR_W   600
#define BAR_H   26
#define BAR_EDGE 2

#define SPIN_CX 512
#define SPIN_CY 364
#define SPIN_R  14
#define SPIN_SEGS 8

/* 8 positions around the spinner ring */
static const int spin_dx[SPIN_SEGS] = { 14, 10, 0, -10, -14, -10, 0, 10 };
static const int spin_dy[SPIN_SEGS] = { 0, -10, -14, -10, 0, 10, 14, 10 };

static void draw_bar(int percent)
{
    uint32 fill_w;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    gfx_fill_rect(BAR_X, BAR_Y, BAR_W, BAR_H, SPLASH_TRACK);
    gfx_draw_rect(BAR_X, BAR_Y, BAR_W, BAR_H, SPLASH_BORDER);

    fill_w = (uint32)((BAR_W - BAR_EDGE * 2) * (uint32)percent / 100);
    if (fill_w > 0) {
        gfx_fill_rect(BAR_X + BAR_EDGE, BAR_Y + BAR_EDGE,
                      fill_w, BAR_H - BAR_EDGE * 2, SPLASH_FILL);
    }
}

/* Draw an 8-segment spinner ring, one segment active based on tick phase. */
static void draw_spinner(void)
{
    uint32 phase = (pit_get_ticks() / 2) % SPIN_SEGS;
    int i;

    /* Clear the ring area */
    gfx_fill_rect(SPIN_CX - SPIN_R - 4, SPIN_CY - SPIN_R - 4,
                  (SPIN_R + 4) * 2, (SPIN_R + 4) * 2, SPLASH_BG);

    for (i = 0; i < SPIN_SEGS; i++) {
        int32 cx = SPIN_CX + spin_dx[i];
        int32 cy = SPIN_CY + spin_dy[i];
        uint32 color = (i == (int)phase) ? SPLASH_FILL : SPLASH_DIM;
        if (cx >= 0 && cy >= 0) {
            gfx_fill_rect((uint32)cx, (uint32)cy, 5, 5, color);
        }
    }
}

static void draw_progress_text(int percent)
{
    char buf[8];
    char *p = buf;
    int n = percent / 10;
    int d = percent % 10;

    gfx_fill_rect(0, 470, 1024, 20, SPLASH_BG);
    if (n > 0) *p++ = (char)('0' + n);
    *p++ = (char)('0' + d);
    *p++ = '%';
    *p = '\0';
    gfx_puts(500, 470, buf, SPLASH_TEXT, SPLASH_BG, 1);
}

static void draw_status(const char *status)
{
    unsigned int len = 0;

    if (!status) return;
    while (status[len]) len++;

    gfx_fill_rect(0, 400, 1024, 20, SPLASH_BG);
    if (len) {
        /* scale-1 text: each char is 8px wide, center on 512 */
        uint32 x = 512 - (uint32)len * 4;
        gfx_puts(x, 400, status, SPLASH_TEXT, SPLASH_BG, 1);
    }
}

static void draw_title(void)
{
    /* "RedLion OS" = 11 chars x scale4 x 8px = 352px wide; centered at 512 -> x=336 */
    gfx_puts(336, 200, "RedLion OS", SPLASH_TEXT, SPLASH_BG, 4);
    /* "Loading..." = 10 chars x scale2 x 8px = 160px wide; centered -> x=432 */
    gfx_puts(432, 310, "Loading...", SPLASH_SUB, SPLASH_BG, 2);
}

void splash_init(void)
{
    gfx_clear(SPLASH_BG);
    draw_title();
    draw_bar(0);
    draw_progress_text(0);
    draw_spinner();
}

void splash_stage(int percent, const char *status, unsigned int hold_ms)
{
    draw_status(status);

    draw_bar(percent);
    draw_progress_text(percent);

    /* Animate the spinner for the hold period */
    {
        uint32 end = pit_get_ticks() + hold_ms / 10;
        while (pit_get_ticks() < end) {
            draw_spinner();
        }
    }
}