#include "wm.h"
#include "../lib/syscall.h"

static wm_window_t windows[WM_MAX_WINDOWS];
static uint32 next_id = 1;
static uint32 focused_id = 0;
static uint32 top_z = 0;

static int32 mouse_x = 512;
static int32 mouse_y = 384;
static int32 mouse_btn = 0;
static int32 prev_mouse_btn = 0;
static int32 prev_mouse_x = 512;
static int32 prev_mouse_y = 384;

static int32 resize_dir = WM_RESIZE_NONE;
static int32 resize_win_idx = -1;
static int32 resize_start_x, resize_start_y;
static int32 resize_orig_x, resize_orig_y;
static uint32 resize_orig_w, resize_orig_h;

static uint32 *lfb_ptr;
static uint32 screen_w, screen_h, lfb_pitch;

static uint8 start_menu_open = 0;
static uint32 uptime_seconds = 0;
static uint32 tick_counter = 0;
static uint32 last_clock_render = 0;
static int32 fm_scroll = 0;

extern "C" uint32 pit_get_ticks(void);

#define COL_BG           0x001E1E2E
#define COL_TITLE_F      0x00FFFFFF
#define COL_TITLE_U      0x00888899
#define COL_TITLEBAR_F   0x002A2A4A
#define COL_TITLEBAR_U   0x00252535
#define COL_BORDER_F     0x004488CC
#define COL_BORDER_U     0x00444455
#define COL_RED          0x00CC4444
#define COL_RED_HOVER    0x00EE5555
#define COL_GREEN        0x0044CC44
#define COL_BLUE         0x004488CC
#define COL_YELLOW       0x00CCCC44
#define COL_WHITE        0x00FFFFFF
#define COL_BLACK        0x00000000
#define COL_DARK         0x00161626
#define COL_CLIENT       0x0012121E
#define COL_SHADOW       0x00000000
#define COL_TASKBAR      0x00161626
#define COL_TASKBAR_BTN  0x002A2A4A
#define COL_TASKBAR_ACT  0x004488CC
#define COL_TASKBAR_TXT  0x00BBBBCC
#define COL_MENU_BG      0x001E1E2E
#define COL_MENU_HOVER   0x00334466
#define COL_MENU_SEP     0x00444455
#define COL_MENU_TXT     0x00CCCCDD
#define COL_MENU_HEAD    0x004488CC
#define COL_SCROLLBAR    0x00333344
#define COL_SCROLL_THUMB 0x00555566
#define COL_BTN_HOVER   0x003A3A5A

/* Desktop icon / gradient / button palette */
#define COL_DESKTOP_TOP  0x00162030
#define COL_DESKTOP_BOT  0x000E1420
#define COL_ICON_BG      0x00222840
#define COL_ICON_HOVER   0x00334060
#define COL_BTN_CLOSE    0x00DD4444
#define COL_BTN_CLOSE_H  0x00FF5555
#define COL_BTN_MIN      0x00DDAA33
#define COL_BTN_MIN_H    0x00FFCC55
#define COL_BTN_MAX      0x0044BB66
#define COL_BTN_MAX_H    0x0055DD88
#define COL_ACCENT       0x004488CC
#define COL_FG_DIM       0x00666688

static const char *app_names[WM_APP_COUNT] = {
    "File Manager", "Terminal", "About"
};

/* Desktop icon layout */
#define DESKTOP_ICON_SIZE  48
#define DESKTOP_ICON_PAD   24
#define DESKTOP_ICON_START_Y 32
struct desktop_icon { int32 x, y; uint8 app_type; const char *label; };
static struct desktop_icon desk_icons[WM_APP_COUNT] = {
    { 32, DESKTOP_ICON_START_Y, WM_APP_FILE_MANAGER, "Files" },
    { 32, DESKTOP_ICON_START_Y + DESKTOP_ICON_SIZE + DESKTOP_ICON_PAD, WM_APP_TERMINAL, "Terminal" },
    { 32, DESKTOP_ICON_START_Y + (DESKTOP_ICON_SIZE + DESKTOP_ICON_PAD) * 2, WM_APP_ABOUT, "About" },
};

/* File Manager entries (scrollable) */
static const char *fm_entries[] = {
    "[DIR]  apps/",
    "[DIR]  system/",
    "[DIR]  users/",
    "[FILE] readme.txt",
    "[FILE] config.cfg",
    "[DIR]  documents/",
    "[DIR]  downloads/",
    "[FILE] notes.txt",
    "[FILE] todo.md",
    "[DIR]  projects/",
    "[DIR]  logs/",
    "[FILE] boot.log",
    "[FILE] hydra.ini",
    "[DIR]  media/",
    "[DIR]  scripts/",
    "[FILE] run.sh",
    "[FILE] backup.zip",
    "[DIR]  fonts/",
    "[DIR]  themes/",
    "[FILE] theme.cfg",
    "[FILE] readme2.txt",
    0
};
static const uint32 fm_entry_count = (uint32)(sizeof(fm_entries) / sizeof(fm_entries[0]) - 1);

static inline void put_px(uint32 x, uint32 y, uint32 c)
{
    if (x < screen_w && y < screen_h)
        lfb_ptr[y * lfb_pitch + x] = c;
}

static void fill_rect(int32 x, int32 y, uint32 w, uint32 h, uint32 c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w == 0 || h == 0) return;
    if ((uint32)x >= screen_w || (uint32)y >= screen_h) return;
    if (w > screen_w - (uint32)x) w = screen_w - (uint32)x;
    if (h > screen_h - (uint32)y) h = screen_h - (uint32)y;

    uint32 row;
    for (row = 0; row < h; row++) {
        uint32 *line = &lfb_ptr[(uint32)(y + row) * lfb_pitch + (uint32)x];
        uint32 col;
        for (col = 0; col < w; col++) line[col] = c;
    }
}

static void fill_rect_gradient_h(int32 x, int32 y, uint32 w, uint32 h,
                                  uint32 c_left, uint32 c_right)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w == 0 || h == 0) return;
    if ((uint32)x >= screen_w || (uint32)y >= screen_h) return;
    if (w > screen_w - (uint32)x) w = screen_w - (uint32)x;
    if (h > screen_h - (uint32)y) h = screen_h - (uint32)y;

    uint32 col_lut[1040];
    uint32 col;
    uint32 dl = w > 1 ? w - 1 : 1;
    int32 dr_chan = (int32)((c_right & 0xFF) - (c_left & 0xFF));
    int32 dg_chan = (int32)(((c_right >> 8) & 0xFF) - ((c_left >> 8) & 0xFF));
    int32 db_chan = (int32)(((c_right >> 16) & 0xFF) - ((c_left >> 16) & 0xFF));
    for (col = 0; col < w; col++) {
        uint32 r = (c_left & 0xFF) + ((uint32)(dr_chan * (int32)col)) / dl;
        uint32 g = ((c_left >> 8) & 0xFF) + ((uint32)(dg_chan * (int32)col)) / dl;
        uint32 b = ((c_left >> 16) & 0xFF) + ((uint32)(db_chan * (int32)col)) / dl;
        col_lut[col] = r | (g << 8) | (b << 16);
    }

    uint32 row;
    for (row = 0; row < h; row++) {
        uint32 *line = &lfb_ptr[(uint32)(y + row) * lfb_pitch + (uint32)x];
        for (col = 0; col < w; col++) line[col] = col_lut[col];
    }
}

static void draw_shadow(int32 x, int32 y, uint32 w, uint32 h)
{
    uint32 row, col;
    uint32 dark = 0x00101020;
    for (row = 0; row < 6; row++) {
        for (col = 0; col < w + 6; col++) {
            int32 px = x + (int32)col + 3;
            int32 py = y + (int32)h + (int32)row;
            if (px >= 0 && py >= 0 && (uint32)px < screen_w && (uint32)py < screen_h) {
                put_px((uint32)px, (uint32)py, dark);
            }
        }
    }
    for (row = 0; row < h + 6; row++) {
        for (col = 0; col < 6; col++) {
            int32 px = x + (int32)w + (int32)col;
            int32 py = y + (int32)row;
            if (px >= 0 && py >= 0 && (uint32)px < screen_w && (uint32)py < screen_h) {
                put_px((uint32)px, (uint32)py, dark);
            }
        }
    }
}

static const unsigned char font8x16[96][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0x18,0x3C,0x3C,0x3C,0x18,0x18,0,0,0x18,0x18,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0x6C,0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x6C,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0x66,0x3C,0xFF,0x3C,0x66,0,0,0,0,0,0,0},
    {0,0,0,0,0x18,0x18,0x7E,0x18,0x18,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0x18,0x18,0x30,0,0,0},
    {0,0,0,0,0,0,0,0xFE,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0x18,0x18,0,0,0,0},
    {0,0,0,0,2,6,0x18,0x30,0x60,0xC0,0x80,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x7E,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,6,0x0C,0x18,0x30,0xC0,0xFE,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,6,0x3C,6,6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0,0,0,0,0},
    {0,0,0,0xFE,0xC0,0xFC,6,6,6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x38,0x60,0xC0,0xFC,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0xFE,0xC6,6,0x0C,0x18,0x30,0x30,0x30,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0xC6,0x7E,6,6,0x0C,0x78,0,0,0,0,0},
    {0,0,0,0,0,0x18,0x18,0,0,0x18,0x18,0,0,0,0,0},
    {0,0,0,0,0,0x18,0x18,0,0,0x18,0x18,0x30,0,0,0,0},
    {0,0,0,0,6,0x0C,0x18,0x30,0x18,0x0C,6,0,0,0,0,0},
    {0,0,0,0,0,0x7E,0,0,0x7E,0,0,0,0,0,0,0},
    {0,0,0,0,0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0x0C,0x18,0x18,0,0x18,0x18,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0x10,0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0,0,0,0,0},
    {0,0,0,0xFC,0x66,0x7C,0x66,0x66,0x66,0x66,0xFC,0,0,0,0,0},
    {0,0,0,0x3C,0x66,0xC2,0xC0,0xC0,0xC2,0x66,0x3C,0,0,0,0,0},
    {0,0,0,0xF8,0x6C,0x66,0x66,0x66,0x66,0x6C,0xF8,0,0,0,0,0},
    {0,0,0,0xFE,0x62,0x68,0x78,0x68,0x62,0x66,0xFE,0,0,0,0,0},
    {0,0,0,0xFE,0x62,0x68,0x78,0x68,0x60,0x60,0xF0,0,0,0,0,0},
    {0,0,0,0x3C,0x66,0xC0,0xC0,0xDE,0xC6,0x66,0x3A,0,0,0,0,0},
    {0,0,0,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0,0,0,0,0},
    {0,0,0,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0},
    {0,0,0,0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0xCC,0x78,0,0,0,0,0},
    {0,0,0,0xE6,0x66,0x6C,0x78,0x78,0x6C,0x66,0xE6,0,0,0,0,0},
    {0,0,0,0xF0,0x60,0x60,0x60,0x60,0x62,0x66,0xFE,0,0,0,0,0},
    {0,0,0,0xC6,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0,0,0,0,0},
    {0,0,0,0xC6,0xE6,0xFE,0xDE,0xC6,0xC6,0xC6,0xC6,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0xFC,0x66,0x7C,0x60,0x60,0x60,0x60,0xF0,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0C,0x0E,0,0,0},
    {0,0,0,0xFC,0x66,0x7C,0x6C,0x66,0x66,0x66,0xE6,0,0,0,0,0},
    {0,0,0,0x7C,0xC6,0x60,0x38,0x0C,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0xFF,0xDB,0x99,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0},
    {0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0,0,0,0,0},
    {0,0,0,0xC6,0xD6,0xD6,0xFE,0x6C,0x6C,0,0,0,0,0,0,0},
    {0,0,0,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0,0,0,0,0,0,0},
    {0,0,0,0xCC,0xCC,0x78,0x30,0x30,0x78,0,0,0,0,0,0,0},
    {0,0,0,0xFE,0xC6,0x86,0x0C,0x18,0x60,0xC6,0xFE,0,0,0,0,0},
    {0,0,0,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0,0,0,0,0},
    {0,0,0,0,0x80,0xC0,0x60,0x30,0x18,0x0C,2,0,0,0,0,0},
    {0,0,0,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0,0,0,0,0},
    {0,0,0x10,0x38,0x6C,0xC6,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0xFF,0,0,0,0},
    {0x30,0x30,0x18,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0x78,0x0C,0x7C,0xCC,0xCC,0x76,0,0,0,0,0},
    {0,0,0,0xE0,0x60,0x78,0x6C,0x66,0x66,0x66,0x7C,0,0,0,0,0},
    {0,0,0,0,0,0x7C,0xC6,0xC0,0xC0,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x1C,0x0C,0x3C,0x6C,0xCC,0xCC,0xCC,0x76,0,0,0,0,0},
    {0,0,0,0,0,0x7C,0xC6,0xFE,0xC0,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x38,0x6C,0x64,0xF0,0x60,0x60,0x60,0xF0,0,0,0,0,0},
    {0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,0,0},
    {0,0,0,0xE0,0x60,0x6C,0x76,0x66,0x66,0x66,0xE6,0,0,0,0,0},
    {0,0,0,0x18,0x18,0,0x38,0x18,0x18,0x18,0x3C,0,0,0,0,0},
    {0,0,0,6,6,0,0x0E,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0,0},
    {0,0,0,0xE0,0x60,0x66,0x6C,0x78,0x6C,0x66,0xE6,0,0,0,0,0},
    {0,0,0,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0},
    {0,0,0,0,0,0xEC,0xFE,0xD6,0xD6,0xD6,0xC6,0,0,0,0,0},
    {0,0,0,0,0,0xDC,0x66,0x66,0x66,0x66,0x66,0,0,0,0,0},
    {0,0,0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0,0,0xDC,0x66,0x66,0x66,0x66,0x7C,0x60,0xF0,0,0,0},
    {0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0x1E,0,0,0},
    {0,0,0,0,0,0xDC,0x76,0x60,0x60,0x60,0xF0,0,0,0,0,0},
    {0,0,0,0,0,0x7C,0xC6,0x60,0x38,0xC6,0x7C,0,0,0,0,0},
    {0,0,0,0x10,0x30,0xFC,0x30,0x30,0x30,0x36,0x1C,0,0,0,0,0},
    {0,0,0,0,0,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0,0,0,0,0},
    {0,0,0,0,0,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0,0,0,0,0},
    {0,0,0,0,0,0xC6,0xD6,0xD6,0xD6,0xFE,0x6C,0,0,0,0,0},
    {0,0,0,0,0,0xC6,0x6C,0x38,0x6C,0xC6,0,0,0,0,0,0},
    {0,0,0,0,0,0xC6,0xC6,0xC6,0xC6,0x7E,6,0x0C,0xF8,0,0,0},
    {0,0,0,0,0,0xFE,0xCC,0x18,0x30,0xC6,0xFE,0,0,0,0,0},
    {0,0,0,0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0,0,0,0,0,0},
    {0,0,0,0x18,0x18,0x18,0,0x18,0x18,0x18,0,0,0,0,0,0},
    {0,0,0,0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0,0,0,0,0,0},
    {0,0,0,0x76,0xDC,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void draw_char(int32 x, int32 y, char ch, uint32 fg, uint32 bg)
{
    unsigned int idx = (unsigned int)ch;
    if (idx < 32 || idx > 127) return;
    idx -= 32;
    const unsigned char *g = font8x16[idx < 96 ? idx : 0];
    unsigned int gy, gx;
    for (gy = 0; gy < 16; gy++) {
        unsigned char row = g[gy];
        for (gx = 0; gx < 8; gx++) {
            uint32 c = (row & (0x80 >> gx)) ? fg : bg;
            put_px((uint32)(x + (int32)gx), (uint32)(y + (int32)gy), c);
        }
    }
}

static void draw_str(int32 x, int32 y, const char *s, uint32 fg, uint32 bg)
{
    while (*s) {
        draw_char(x, y, *s, fg, bg);
        x += 8;
        s++;
    }
}

static int str_width(const char *s)
{
    int len = 0;
    while (*s) { len++; s++; }
    return len * 8;
}

static void draw_str_centered(int32 cx, int32 y, const char *s, uint32 fg, uint32 bg)
{
    int w = str_width(s);
    draw_str(cx - w / 2, y, s, fg, bg);
}

static uint32 color_for_app(uint8 app_type)
{
    switch (app_type) {
        case WM_APP_FILE_MANAGER: return COL_GREEN;
        case WM_APP_TERMINAL:     return COL_BLUE;
        case WM_APP_ABOUT:        return COL_YELLOW;
        default:                  return COL_WHITE;
    }
}

static void draw_cursor_arrow(int32 cx, int32 cy)
{
    int row, col;
    for (row = 0; row < 16; row++) {
        for (col = 0; col <= row && col < 10; col++) {
            int32 px = cx + col;
            int32 py = cy + row;
            if (px < 0 || py < 0 || (uint32)px >= screen_w || (uint32)py >= screen_h)
                continue;
            int inside = 1;
            if (row < 15 && col >= row) inside = 0;
            int outline = 0;
            int dx, dy;
            for (dy = -1; dy <= 1 && !outline; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = col + dx;
                    int ny = row + dy;
                    int ninside = (ny >= 0 && ny < 16 && nx >= 0 && nx <= ny && nx < 10);
                    if (inside && !ninside) { outline = 1; break; }
                }
            }
            if (outline)
                lfb_ptr[py * lfb_pitch + px] = COL_BLACK;
            else if (inside)
                lfb_ptr[py * lfb_pitch + px] = COL_WHITE;
        }
    }
}

static void draw_cursor_resize_h(int32 cx, int32 cy)
{
    fill_rect(cx - 7, cy - 1, 14, 2, COL_WHITE);
    fill_rect(cx - 1, cy - 7, 2, 14, COL_WHITE);
    fill_rect(cx - 7, cy, 5, 1, COL_BLACK);
    fill_rect(cx + 2, cy, 5, 1, COL_BLACK);
    fill_rect(cx, cy - 7, 1, 5, COL_BLACK);
    fill_rect(cx, cy + 2, 1, 5, COL_BLACK);
}

static void draw_cursor_resize_v(int32 cx, int32 cy)
{
    fill_rect(cx - 1, cy - 7, 2, 14, COL_WHITE);
    fill_rect(cx - 7, cy - 1, 14, 2, COL_WHITE);
    fill_rect(cx, cy - 7, 1, 5, COL_BLACK);
    fill_rect(cx, cy + 2, 1, 5, COL_BLACK);
    fill_rect(cx - 7, cy, 5, 1, COL_BLACK);
    fill_rect(cx + 2, cy, 5, 1, COL_BLACK);
}

static void draw_cursor_resize_tl(int32 cx, int32 cy)
{
    int i;
    for (i = 0; i < 10; i++) {
        put_px(cx - i, cy - i, COL_WHITE);
        put_px(cx - i + 1, cy - i, COL_BLACK);
        put_px(cx - i, cy - i + 1, COL_BLACK);
    }
}

static void draw_cursor_resize_br(int32 cx, int32 cy)
{
    int i;
    for (i = 0; i < 10; i++) {
        put_px(cx + i, cy + i, COL_WHITE);
        put_px(cx + i - 1, cy + i, COL_BLACK);
        put_px(cx + i, cy + i - 1, COL_BLACK);
    }
}

static void draw_cursor_resize_tr(int32 cx, int32 cy)
{
    int i;
    for (i = 0; i < 10; i++) {
        put_px(cx + i, cy - i, COL_WHITE);
        put_px(cx + i - 1, cy - i, COL_BLACK);
        put_px(cx + i, cy - i + 1, COL_BLACK);
    }
}

static void draw_cursor_resize_bl(int32 cx, int32 cy)
{
    int i;
    for (i = 0; i < 10; i++) {
        put_px(cx - i, cy + i, COL_WHITE);
        put_px(cx - i + 1, cy + i, COL_BLACK);
        put_px(cx - i, cy + i - 1, COL_BLACK);
    }
}

static void draw_cursor_at(int32 cx, int32 cy, int32 resize)
{
    switch (resize) {
        case WM_RESIZE_LEFT:
        case WM_RESIZE_RIGHT:
            draw_cursor_resize_h(cx, cy);
            break;
        case WM_RESIZE_TOP:
        case WM_RESIZE_BOTTOM:
            draw_cursor_resize_v(cx, cy);
            break;
        case WM_RESIZE_TL:
            draw_cursor_resize_tl(cx, cy);
            break;
        case WM_RESIZE_BR:
            draw_cursor_resize_br(cx, cy);
            break;
        case WM_RESIZE_TR:
            draw_cursor_resize_tr(cx, cy);
            break;
        case WM_RESIZE_BL:
            draw_cursor_resize_bl(cx, cy);
            break;
        default:
            draw_cursor_arrow(cx, cy);
            break;
    }
}

/* Filled circle helper: fills pixels within radius r of (cx,cy). */
static void fill_circle(int32 cx, int32 cy, int32 r, uint32 color)
{
    int32 dy;
    for (dy = -r; dy <= r; dy++) {
        int32 dx;
        for (dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                int32 px = cx + dx;
                int32 py = cy + dy;
                if (px >= 0 && py >= 0 && (uint32)px < screen_w && (uint32)py < screen_h)
                    put_px((uint32)px, (uint32)py, color);
            }
        }
    }
}

/* Fill rectangle inside a window's surface buffer */
static void fill_rect_in_surface(wm_window_t *w, uint32 x, uint32 y, uint32 rw, uint32 rh, uint32 c)
{
    uint32 row;
    for (row = 0; row < rh && y + row < w->surface_h; row++) {
        uint32 col;
        for (col = 0; col < rw && x + col < w->surface_w; col++) {
            w->surface[(y + row) * w->surface_w + x + col] = c;
        }
    }
}

/* Draw a filled rounded rectangle */
static void fill_rounded_rect(int32 x, int32 y, uint32 w, uint32 h, uint32 color)
{
    fill_rect(x + 2, y, w - 4, h, color);
    fill_rect(x, y + 2, w, h - 4, color);
    fill_circle(x + 2, y + 2, 2, color);
    fill_circle(x + (int32)w - 3, y + 2, 2, color);
    fill_circle(x + 2, y + (int32)h - 3, 2, color);
    fill_circle(x + (int32)w - 3, y + (int32)h - 3, 2, color);
}

static wm_window_t *find_window(uint32 id)
{
    int i;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (windows[i].id == id && windows[i].flags) return &windows[i];
    }
    return 0;
}

static int find_window_idx(uint32 id)
{
    int i;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (windows[i].id == id && windows[i].flags) return i;
    }
    return -1;
}



static int hit_test(int32 mx, int32 my)
{
    int i;
    int best = -1;
    int32 best_z = -1;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!(windows[i].flags & WM_WIN_VISIBLE)) continue;
        if (windows[i].flags & WM_WIN_MINIMIZED) continue;
        if (mx >= windows[i].x && mx < windows[i].x + (int32)windows[i].w &&
            my >= windows[i].y && my < windows[i].y + (int32)windows[i].h) {
            if (best < 0 || windows[i].z_order > best_z) {
                best_z = windows[i].z_order;
                best = i;
            }
        }
    }
    return best;
}

static int32 get_resize_dir(int32 mx, int32 my, int win_idx)
{
    wm_window_t *w = &windows[win_idx];
    if (!(w->flags & WM_WIN_VISIBLE) || (w->flags & WM_WIN_MINIMIZED)) return WM_RESIZE_NONE;
    if (w->flags & WM_WIN_MAXIMIZED) return WM_RESIZE_NONE;

    int32 zx = w->x;
    int32 zy = w->y;
    uint32 zw = w->w;
    uint32 zh = w->h;

    int on_left = (mx >= zx && mx < zx + WM_RESIZE_ZONE);
    int on_right = (mx >= zx + (int32)zw - WM_RESIZE_ZONE && mx < zx + (int32)zw);
    int on_top = (my >= zy && my < zy + WM_RESIZE_ZONE);
    int on_bottom = (my >= zy + (int32)zh - WM_RESIZE_ZONE && my < zy + (int32)zh);

    if (on_top && on_left) return WM_RESIZE_TL;
    if (on_top && on_right) return WM_RESIZE_TR;
    if (on_bottom && on_left) return WM_RESIZE_BL;
    if (on_bottom && on_right) return WM_RESIZE_BR;
    if (on_left) return WM_RESIZE_LEFT;
    if (on_right) return WM_RESIZE_RIGHT;
    if (on_top) return WM_RESIZE_TOP;
    if (on_bottom) return WM_RESIZE_BOTTOM;

    return WM_RESIZE_NONE;
}

static void focus_top(void)
{
    int i;
    int best = -1;
    int32 best_z = -1;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!(windows[i].flags & WM_WIN_VISIBLE)) continue;
        if (windows[i].flags & WM_WIN_MINIMIZED) continue;
        if (best < 0 || windows[i].z_order > best_z) {
            best_z = windows[i].z_order;
            best = i;
        }
    }
    if (best >= 0) {
        int j;
        for (j = 0; j < WM_MAX_WINDOWS; j++) windows[j].focused = 0;
        windows[best].focused = 1;
        focused_id = windows[best].id;
    }
}

static int32 win_content_x(wm_window_t *w) { return w->x + 1; }
static int32 win_content_y(wm_window_t *w) { return w->y + WM_TITLEBAR_H + 1; }
static uint32 win_content_w(wm_window_t *w) { return w->w - 2; }
static uint32 win_content_h(wm_window_t *w) { return w->h - WM_TITLEBAR_H - 2; }

static void wm_update_surface_size(wm_window_t *w)
{
    uint32 cw = win_content_w(w);
    uint32 ch = win_content_h(w);
    if (cw > WM_MAX_SURFACE_W) cw = WM_MAX_SURFACE_W;
    if (ch > WM_MAX_SURFACE_H) ch = WM_MAX_SURFACE_H;
    w->surface_w = cw;
    w->surface_h = ch;
}

static void draw_window(wm_window_t *win)
{
    int32 wx = win->x;
    int32 wy = win->y;
    uint32 ww = win->w;
    uint32 wh = win->h;
    uint32 border = win->focused ? COL_BORDER_F : COL_BORDER_U;
    uint32 app_col = color_for_app(win->app_type);

    draw_shadow(wx, wy, ww, wh);

    fill_rect(wx, wy, ww, 1, border);
    fill_rect(wx, wy + (int32)wh - 1, ww, 1, border);
    fill_rect(wx, wy, 1, wh, border);
    fill_rect(wx + (int32)ww - 1, wy, 1, wh, border);

    uint32 c_left, c_right;
    if (win->focused) {
        uint32 r1 = (COL_TITLEBAR_F & 0xFF) + (((app_col & 0xFF) - (COL_TITLEBAR_F & 0xFF)) * 2) / 5;
        uint32 g1 = ((COL_TITLEBAR_F >> 8) & 0xFF) + ((((app_col >> 8) & 0xFF) - ((COL_TITLEBAR_F >> 8) & 0xFF)) * 2) / 5;
        uint32 b1 = ((COL_TITLEBAR_F >> 16) & 0xFF) + ((((app_col >> 16) & 0xFF) - ((COL_TITLEBAR_F >> 16) & 0xFF)) * 2) / 5;
        c_left = r1 | (g1 << 8) | (b1 << 16);
        c_right = COL_TITLEBAR_F;
    } else {
        c_left = COL_TITLEBAR_U;
        c_right = COL_TITLEBAR_U;
    }
    fill_rect_gradient_h(wx + 1, wy + 1, ww - 2, WM_TITLEBAR_H, c_left, c_right);

    uint32 tc = win->focused ? COL_TITLE_F : COL_TITLE_U;
    draw_str(wx + 10, wy + 6, win->title, tc, 0);

    int32 by = wy + 2;
    int32 btn_r = WM_BTN_SIZE / 2 - 2;

    /* Close button (red circle) */
    int32 cbx = wx + (int32)ww - WM_BTN_SIZE - 4;
    int32 cbcx = cbx + WM_BTN_SIZE / 2;
    int32 cbcy = by + WM_BTN_SIZE / 2;
    uint32 close_bg = COL_BTN_CLOSE;
    if (mouse_x >= cbx && mouse_x < cbx + WM_BTN_SIZE &&
        mouse_y >= by && mouse_y < by + WM_BTN_SIZE) {
        close_bg = COL_BTN_CLOSE_H;
    }
    fill_circle(cbcx, cbcy, btn_r, close_bg);
    fill_rect(cbcx - 3, cbcy - 1, 6, 1, COL_WHITE);
    fill_rect(cbcx - 1, cbcy - 3, 1, 6, COL_WHITE);

    /* Maximize button (green circle) */
    int32 xbx = cbx - WM_BTN_SIZE - 2;
    int32 xbcx = xbx + WM_BTN_SIZE / 2;
    uint32 max_bg = (win->flags & WM_WIN_MAXIMIZED) ? COL_BTN_MAX_H : COL_BTN_MAX;
    if (mouse_x >= xbx && mouse_x < xbx + WM_BTN_SIZE &&
        mouse_y >= by && mouse_y < by + WM_BTN_SIZE) {
        max_bg = COL_BTN_MAX_H;
    }
    fill_circle(xbcx, cbcy, btn_r, max_bg);
    fill_rect(xbcx - 3, cbcy - 3, 6, 4, COL_WHITE);
    fill_rect(xbcx - 3, cbcy - 3, 1, 6, COL_WHITE);

    /* Minimize button (yellow circle) */
    int32 mbx = xbx - WM_BTN_SIZE - 2;
    int32 mbcx = mbx + WM_BTN_SIZE / 2;
    uint32 min_bg = COL_BTN_MIN;
    if (mouse_x >= mbx && mouse_x < mbx + WM_BTN_SIZE &&
        mouse_y >= by && mouse_y < by + WM_BTN_SIZE) {
        min_bg = COL_BTN_MIN_H;
    }
    fill_circle(mbcx, cbcy, btn_r, min_bg);
    fill_rect(mbcx - 3, cbcy + 1, 6, 1, COL_WHITE);

    fill_rect(wx + 1, wy + WM_TITLEBAR_H + 1, ww - 2, wh - WM_TITLEBAR_H - 2, COL_CLIENT);

    uint32 content_w = win_content_w(win);
    uint32 content_h = win_content_h(win);
    if (content_w > 0 && content_h > 0) {
        uint32 row;
        for (row = 0; row < content_h && row < win->surface_h; row++) {
            uint32 col;
            for (col = 0; col < content_w && col < win->surface_w; col++) {
                uint32 px = win->surface[row * win->surface_w + col];
                if (px != 0) {
                    put_px((uint32)(win_content_x(win) + (int32)col),
                           (uint32)(win_content_y(win) + (int32)row), px);
                }
            }
        }
    }
}

static int wm_create(const char *title, int32 x, int32 y, uint32 w, uint32 h, uint8 app_type)
{
    int i;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (windows[i].flags == 0) {
            uint32 j;
            windows[i].id = next_id++;
            windows[i].x = x;
            windows[i].y = y;
            windows[i].w = w;
            windows[i].h = h;
            windows[i].prev_x = x;
            windows[i].prev_y = y;
            windows[i].prev_w = w;
            windows[i].prev_h = h;
            windows[i].flags = WM_WIN_VISIBLE | WM_WIN_DECORATED;
            windows[i].z_order = top_z++;
            windows[i].dirty = 1;
            windows[i].focused = 0;
            windows[i].app_type = app_type;
            wm_update_surface_size(&windows[i]);
            windows[i].needs_surface_clear = 1;
            for (j = 0; j < WM_MAX_TITLE - 1 && title[j]; j++)
                windows[i].title[j] = title[j];
            windows[i].title[j] = '\0';
            for (j = 0; j < WM_MAX_SURFACE; j++)
                windows[i].surface[j] = 0;
            focus_top();
            return (int)windows[i].id;
        }
    }
    return -1;
}

static void wm_destroy(uint32 id)
{
    wm_window_t *w = find_window(id);
    if (w) {
        w->flags = 0;
        w->id = 0;
        if (focused_id == id) focus_top();
    }
}

static void wm_minimize(uint32 id)
{
    wm_window_t *w = find_window(id);
    if (w) {
        w->flags |= WM_WIN_MINIMIZED;
        if (w->focused) focus_top();
    }
}

static void wm_maximize(uint32 id)
{
    wm_window_t *w = find_window(id);
    if (!w) return;

    if (w->flags & WM_WIN_MAXIMIZED) {
        w->flags &= ~WM_WIN_MAXIMIZED;
        w->x = w->prev_x;
        w->y = w->prev_y;
        w->w = w->prev_w;
        w->h = w->prev_h;
    } else {
        w->prev_x = w->x;
        w->prev_y = w->y;
        w->prev_w = w->w;
        w->prev_h = w->h;
        w->flags |= WM_WIN_MAXIMIZED;
        w->x = 0;
        w->y = 0;
        w->w = screen_w;
        w->h = screen_h - WM_TASKBAR_H;
    }
    wm_update_surface_size(w);
    w->needs_surface_clear = 1;
}

static void clear_surface(wm_window_t *w)
{
    uint32 i;
    for (i = 0; i < WM_MAX_SURFACE; i++)
        w->surface[i] = 0;
    w->needs_surface_clear = 0;
}

static void draw_taskbar(void)
{
    int32 tb_y = (int32)(screen_h - WM_TASKBAR_H);

    fill_rect(0, tb_y, screen_w, WM_TASKBAR_H, COL_TASKBAR);
    fill_rect(0, tb_y, screen_w, 1, COL_BORDER_F);

    /* Gradient highlight at the top edge of the taskbar */
    uint32 hl0 = 0x003A4A6A;
    uint32 hl1 = 0x00222436;
    int32 i;
    for (i = 0; i < 3; i++) {
        int32 r = (int32)(hl0 & 0xFF) + (((int32)(hl1 & 0xFF) - (int32)(hl0 & 0xFF)) * i) / 3;
        int32 g = (int32)((hl0 >> 8) & 0xFF) + (((int32)((hl1 >> 8) & 0xFF) - (int32)((hl0 >> 8) & 0xFF)) * i) / 3;
        int32 b = (int32)((hl0 >> 16) & 0xFF) + (((int32)((hl1 >> 16) & 0xFF) - (int32)((hl0 >> 16) & 0xFF)) * i) / 3;
        fill_rect(0, tb_y + 1 + i, screen_w - 1, 1, (uint32)r | ((uint32)g << 8) | ((uint32)b << 16));
    }

    /* Start button with logo mark */
    int32 btn_w = 86;
    uint32 start_bg = COL_TASKBAR_BTN;
    if (mouse_x >= 0 && mouse_x < btn_w &&
        mouse_y >= tb_y && mouse_y < (int32)screen_h) {
        start_bg = COL_TASKBAR_ACT;
    }
    fill_rounded_rect(2, tb_y + 4, (uint32)btn_w, WM_TASKBAR_H - 8, start_bg);
    /* Logo: 4-pane window mark */
    fill_rect(12, tb_y + 10, 16, 16, COL_ACCENT);
    fill_rect(13, tb_y + 11, 14, 14, 0x000E1420);
    fill_rect(16, tb_y + 14, 4, 4, COL_ACCENT);
    fill_rect(22, tb_y + 14, 4, 4, COL_ACCENT);
    draw_str(34, tb_y + 10, "Start", COL_WHITE, start_bg);

    int32 x_off = btn_w + 6;
    int k;
    for (k = 0; k < WM_MAX_WINDOWS; k++) {
        wm_window_t *win = &windows[k];
        if (!(win->flags & WM_WIN_VISIBLE)) continue;
        if (win->id == 0) continue;

        int32 bw = str_width(win->title) + 24;
        if (bw < 60) bw = 60;
        if (x_off + bw > (int32)screen_w - 120) break;

        uint32 btn_bg = win->focused ? COL_TASKBAR_ACT : COL_TASKBAR_BTN;
        if (mouse_x >= x_off && mouse_x < x_off + bw &&
            mouse_y >= tb_y && mouse_y < (int32)screen_h) {
            btn_bg = COL_BTN_HOVER;
            if (win->focused) btn_bg = COL_TASKBAR_ACT;
        }
        fill_rect(x_off, tb_y + 5, (uint32)bw, WM_TASKBAR_H - 10, btn_bg);
        fill_rect(x_off, tb_y + 5, 1, WM_TASKBAR_H - 10, COL_BORDER_F);

        uint32 dot_col = color_for_app(win->app_type);
        fill_circle(x_off + 9, tb_y + 9, 4, dot_col);

        draw_str(x_off + 18, tb_y + 8, win->title, COL_TASKBAR_TXT, btn_bg);

        if (win->focused) {
            fill_rect(x_off + 2, tb_y + (int32)WM_TASKBAR_H - 4, (uint32)bw - 4, 2, COL_ACCENT);
        }

        x_off += bw + 4;
    }

    int32 tray_x = (int32)screen_w - 110;
    fill_rounded_rect(tray_x, tb_y + 4, 108, WM_TASKBAR_H - 8, COL_TASKBAR_BTN);

    uint32 sec = uptime_seconds;
    uint32 m = sec / 60;
    uint32 s = sec % 60;
    char time_str[10];
    time_str[0] = '0' + (char)(m / 10);
    time_str[1] = '0' + (char)(m % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (char)(s / 10);
    time_str[4] = '0' + (char)(s % 10);
    time_str[5] = '\0';
    draw_str(tray_x + 10, tb_y + 10, time_str, COL_TASKBAR_TXT, COL_TASKBAR_BTN);

    draw_str(tray_x + 60, tb_y + 10, "RedLion", COL_BLUE, COL_TASKBAR_BTN);
}

static int taskbar_hit_test(int32 mx, int32 my)
{
    int32 tb_y = (int32)(screen_h - WM_TASKBAR_H);
    if (my < tb_y) return -1;

    if (mx < 80) return -2;

    int32 x_off = 84;
    int i;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!(windows[i].flags & WM_WIN_VISIBLE)) continue;
        if (windows[i].id == 0) continue;

        int32 bw = str_width(windows[i].title) + 24;
        if (bw < 60) bw = 60;
        if (x_off + bw > (int32)screen_w - 120) break;

        if (mx >= x_off && mx < x_off + bw) {
            return (int)windows[i].id;
        }
        x_off += bw + 4;
    }
    return -1;
}

static void draw_start_menu(void)
{
    if (!start_menu_open) return;

    int32 menu_x = 2;
    int32 menu_y = (int32)(screen_h - WM_TASKBAR_H) - 200;
    uint32 menu_w = 180;
    uint32 menu_h = 200;

    fill_rect(menu_x - 2, menu_y - 2, menu_w + 4, menu_h + 4, COL_SHADOW);
    fill_rect(menu_x, menu_y, menu_w, menu_h, COL_MENU_BG);
    fill_rect(menu_x, menu_y, 1, menu_h, COL_BORDER_F);
    fill_rect(menu_x + (int32)menu_w - 1, menu_y, 1, menu_h, COL_BORDER_F);
    fill_rect(menu_x, menu_y, menu_w, 1, COL_BORDER_F);
    fill_rect(menu_x, menu_y + (int32)menu_h - 1, menu_w, 1, COL_BORDER_F);

    draw_str(menu_x + 10, menu_y + 8, "Applications", COL_MENU_HEAD, COL_MENU_BG);
    fill_rect(menu_x + 8, menu_y + 24, menu_w - 16, 1, COL_MENU_SEP);

    int i;
    int32 entry_y = menu_y + 32;
    for (i = 0; i < WM_APP_COUNT; i++) {
        uint32 entry_bg = COL_MENU_BG;
        if (mouse_x >= menu_x && mouse_x < menu_x + (int32)menu_w &&
            mouse_y >= entry_y && mouse_y < entry_y + 24) {
            entry_bg = COL_MENU_HOVER;
        }
        fill_rect(menu_x + 2, entry_y, menu_w - 4, 24, entry_bg);

        uint32 dot = color_for_app((uint8)i);
        fill_rect(menu_x + 12, entry_y + 8, 8, 8, dot);

        draw_str(menu_x + 26, entry_y + 5, app_names[i], COL_MENU_TXT, entry_bg);

        entry_y += 26;
    }

    fill_rect(menu_x + 8, entry_y + 2, menu_w - 16, 1, COL_MENU_SEP);
    entry_y += 10;

    uint32 shut_bg = COL_MENU_BG;
    if (mouse_x >= menu_x && mouse_x < menu_x + (int32)menu_w &&
        mouse_y >= entry_y && mouse_y < entry_y + 24) {
        shut_bg = COL_RED;
    }
    fill_rect(menu_x + 2, entry_y, menu_w - 4, 24, shut_bg);
    draw_str(menu_x + 26, entry_y + 5, "Shutdown", shut_bg == COL_RED ? COL_WHITE : COL_MENU_TXT, shut_bg);

    entry_y += 26;
    uint32 reboot_bg = COL_MENU_BG;
    if (mouse_x >= menu_x && mouse_x < menu_x + (int32)menu_w &&
        mouse_y >= entry_y && mouse_y < entry_y + 24) {
        reboot_bg = COL_MENU_HOVER;
    }
    fill_rect(menu_x + 2, entry_y, menu_w - 4, 24, reboot_bg);
    draw_str(menu_x + 26, entry_y + 5, "Reboot", COL_MENU_TXT, reboot_bg);
}

static int start_menu_hit_test(int32 mx, int32 my)
{
    if (!start_menu_open) return -1;

    int32 menu_x = 2;
    int32 menu_y = (int32)(screen_h - WM_TASKBAR_H) - 200;
    uint32 menu_w = 180;

    if (mx < menu_x || mx >= menu_x + (int32)menu_w) return -1;
    if (my < menu_y) return -1;

    int32 entry_y = menu_y + 32;
    int i;
    for (i = 0; i < WM_APP_COUNT; i++) {
        if (my >= entry_y && my < entry_y + 24) return i;
        entry_y += 26;
    }
    entry_y += 10;
    if (my >= entry_y && my < entry_y + 24) return 100;
    entry_y += 26;
    if (my >= entry_y && my < entry_y + 24) return 101;

    return -1;
}

static void draw_desktop_bg(void)
{
    /* Vertical desktop gradient: darker at the bottom */
    uint32 row;
    int32 r0 = COL_DESKTOP_TOP & 0xFF;
    int32 g0 = (COL_DESKTOP_TOP >> 8) & 0xFF;
    int32 b0 = (COL_DESKTOP_TOP >> 16) & 0xFF;
    int32 r1 = COL_DESKTOP_BOT & 0xFF;
    int32 g1 = (COL_DESKTOP_BOT >> 8) & 0xFF;
    int32 b1 = (COL_DESKTOP_BOT >> 16) & 0xFF;

    for (row = 0; row < screen_h; row++) {
        int32 dr = r1 - r0;
        int32 dg = g1 - g0;
        int32 db = b1 - b0;
        uint32 c = (uint32)(r0 + dr * (int32)row / (int32)screen_h)
                 | ((uint32)(g0 + dg * (int32)row / (int32)screen_h) << 8)
                 | ((uint32)(b0 + db * (int32)row / (int32)screen_h) << 16);
        uint32 *line = &lfb_ptr[row * lfb_pitch];
        uint32 col;
        for (col = 0; col < screen_w; col++) line[col] = c;
    }

    /* Centered watermark text */
    uint32 cx = screen_w / 2;
    uint32 cy = (screen_h - WM_TASKBAR_H) / 2;
    draw_str_centered((int32)cx, (int32)cy - 20, "RedLion OS", 0x00333355, COL_DESKTOP_TOP);
    draw_str_centered((int32)cx, (int32)cy, "2.0.0", 0x00222244, COL_DESKTOP_TOP);

    /* Desktop icons */
    int i;
    for (i = 0; i < WM_APP_COUNT; i++) {
        struct desktop_icon *ic = &desk_icons[i];
        int hovered = (mouse_x >= ic->x && mouse_x < ic->x + DESKTOP_ICON_SIZE &&
                       mouse_y >= ic->y && mouse_y < ic->y + DESKTOP_ICON_SIZE);
        uint32 box_bg = hovered ? COL_ICON_HOVER : COL_ICON_BG;
        uint32 icon_col = color_for_app(ic->app_type);

        fill_rect(ic->x, ic->y, DESKTOP_ICON_SIZE, DESKTOP_ICON_SIZE, box_bg);
        fill_rect(ic->x, ic->y, DESKTOP_ICON_SIZE, 3, icon_col);
        fill_rect(ic->x, ic->y + DESKTOP_ICON_SIZE - 3, DESKTOP_ICON_SIZE, 3, icon_col);
        fill_rect(ic->x, ic->y, 2, DESKTOP_ICON_SIZE, icon_col);
        fill_rect(ic->x + DESKTOP_ICON_SIZE - 2, ic->y, 2, DESKTOP_ICON_SIZE, icon_col);

        /* Simple "app tile" glyph: a filled circle marker + dot grid */
        fill_circle(ic->x + DESKTOP_ICON_SIZE / 2, ic->y + DESKTOP_ICON_SIZE / 2 - 4, 7, icon_col);
        fill_rect(ic->x + DESKTOP_ICON_SIZE / 2 - 2, ic->y + DESKTOP_ICON_SIZE / 2 + 6, 4, 6, icon_col);

        draw_str_centered(ic->x + DESKTOP_ICON_SIZE / 2, ic->y + DESKTOP_ICON_SIZE + 4,
                          ic->label, COL_TASKBAR_TXT, COL_DESKTOP_TOP);
    }
}

/* Returns app_type if the mouse is over a desktop icon, else -1. */
static int desktop_icon_hit_test(int32 mx, int32 my)
{
    int i;
    for (i = 0; i < WM_APP_COUNT; i++) {
        struct desktop_icon *ic = &desk_icons[i];
        if (mx >= ic->x && mx < ic->x + DESKTOP_ICON_SIZE &&
            my >= ic->y && my < ic->y + DESKTOP_ICON_SIZE) {
            return (int)ic->app_type;
        }
    }
    return -1;
}

static void init_terminal_surface(wm_window_t *w)
{
    if (w->needs_surface_clear) clear_surface(w);
    wm_update_surface_size(w);

    w->term_col = 0;
    w->term_row = 0;
    w->term_buf_len = 0;
    w->term_buf[0] = '\0';

    const char *prompt = "> ";
    uint32 i;
    for (i = 0; prompt[i]; i++) {
        uint32 px = i * 8;
        if (px < w->surface_w) {
            const unsigned char *g = font8x16[(unsigned int)prompt[i] - 32];
            uint32 sy;
            for (sy = 0; sy < 14 && sy < w->surface_h; sy++) {
                unsigned char row = g[sy < 16 ? sy : 0];
                uint32 sx;
                for (sx = 0; sx < 8; sx++) {
                    if (row & (0x80 >> sx)) {
                        w->surface[sy * w->surface_w + px + sx] = COL_GREEN;
                    }
                }
            }
        }
    }
    w->term_col = 2;
}

static void term_scroll_up(wm_window_t *w)
{
    uint32 ch_h = 16;
    uint32 max_rows = w->surface_h / ch_h;
    if (max_rows <= 1) return;
    uint32 *dst = w->surface;
    uint32 *src = w->surface + ch_h * w->surface_w;
    uint32 move_rows = (max_rows - 1) * ch_h;
    uint32 i;
    for (i = 0; i < move_rows * w->surface_w; i++)
        dst[i] = src[i];
    uint32 clear_start = move_rows * w->surface_w;
    uint32 clear_count = ch_h * w->surface_w;
    for (i = 0; i < clear_count; i++)
        dst[clear_start + i] = 0;
    w->term_row = max_rows - 1;
}

static void term_write_char(wm_window_t *w, char ch, uint32 color)
{
    uint32 ch_h = 16;
    uint32 max_rows = w->surface_h / ch_h;
    if (max_rows == 0) return;

    if (ch == '\n') {
        w->term_col = 0;
        w->term_row++;
        if (w->term_row >= max_rows)
            term_scroll_up(w);
        return;
    }

    uint32 px = w->term_col * 8;
    if (px + 8 <= w->surface_w && (w->term_row * ch_h + 16) <= w->surface_h) {
        unsigned int idx = (unsigned int)ch;
        if (idx >= 32 && idx <= 127) {
            idx -= 32;
            const unsigned char *g = font8x16[idx < 96 ? idx : 0];
            uint32 sy;
            for (sy = 0; sy < 16; sy++) {
                unsigned char row = g[sy];
                uint32 sx;
                for (sx = 0; sx < 8; sx++) {
                    if (row & (0x80 >> sx))
                        w->surface[(w->term_row * ch_h + sy) * w->surface_w + px + sx] = color;
                }
            }
        }
    }
    w->term_col++;

    if (w->term_col * 8 + 8 > w->surface_w) {
        w->term_col = 0;
        w->term_row++;
        if (w->term_row >= max_rows)
            term_scroll_up(w);
    }
}

static void term_write_str(wm_window_t *w, const char *s, uint32 color)
{
    while (*s) {
        term_write_char(w, *s, color);
        s++;
    }
}

static void term_draw_prompt(wm_window_t *w)
{
    term_write_str(w, "> ", COL_GREEN);
}

static void term_execute(wm_window_t *w)
{
    w->term_buf[w->term_buf_len] = '\0';
    const char *cmd = w->term_buf;

    term_write_str(w, "\n", COL_WHITE);

    if (cmd[0] == '\0') {
    } else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0') {
        term_write_str(w, "Available commands:\n", COL_WHITE);
        term_write_str(w, "  help    - show this help\n", COL_YELLOW);
        term_write_str(w, "  clear   - clear the screen\n", COL_YELLOW);
        term_write_str(w, "  echo X  - print X\n", COL_YELLOW);
        term_write_str(w, "  about   - show system info\n", COL_YELLOW);
        term_write_str(w, "  uptime  - show uptime\n", COL_YELLOW);
        term_write_str(w, "  reboot  - restart the system\n", COL_YELLOW);
    } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0') {
        uint32 i;
        for (i = 0; i < w->surface_w * w->surface_h; i++)
            w->surface[i] = 0;
        w->term_col = 0;
        w->term_row = 0;
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        term_write_str(w, cmd + 5, COL_WHITE);
        term_write_str(w, "\n", COL_WHITE);
    } else if (cmd[0] == 'a' && cmd[1] == 'b' && cmd[2] == 'o' && cmd[3] == 'u' && cmd[4] == 't' && cmd[5] == '\0') {
        term_write_str(w, "RedLion OS v2.0.0\n", COL_YELLOW);
        term_write_str(w, "Micro-kernel with graphical desktop\n", COL_WHITE);
        term_write_str(w, "C/C++17/x86 ASM | 1024x768x32\n", COL_WHITE);
    } else if (cmd[0] == 'u' && cmd[1] == 'p' && cmd[2] == 't' && cmd[3] == 'i' && cmd[4] == 'm' && cmd[5] == 'e' && cmd[6] == '\0') {
        uint32 sec = uptime_seconds;
        uint32 m = sec / 60;
        uint32 s = sec % 60;
        char time_str[10];
        time_str[0] = '0' + (char)(m / 10);
        time_str[1] = '0' + (char)(m % 10);
        time_str[2] = ':';
        time_str[3] = '0' + (char)(s / 10);
        time_str[4] = '0' + (char)(s % 10);
        time_str[5] = '\0';
        term_write_str(w, time_str, COL_WHITE);
        term_write_str(w, "\n", COL_WHITE);
    } else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o' && cmd[4] == 'o' && cmd[5] == 't' && cmd[6] == '\0') {
        term_write_str(w, "Rebooting...\n", COL_RED);
        sys_reboot();
    } else {
        term_write_str(w, "Unknown command: ", COL_RED);
        term_write_str(w, cmd, COL_WHITE);
        term_write_str(w, "\n", COL_WHITE);
    }

    w->term_buf_len = 0;
    w->term_buf[0] = '\0';
    term_draw_prompt(w);
}

static void terminal_putchar(wm_window_t *w, char ch)
{
    uint32 ch_h = 16;
    uint32 max_rows = w->surface_h / ch_h;
    if (max_rows == 0) return;

    if (w->needs_surface_clear) {
        w->term_col = 0;
        w->term_row = 0;
        w->term_buf_len = 0;
        w->term_buf[0] = '\0';
        clear_surface(w);
        init_terminal_surface(w);
    }

    if (ch == '\n') {
        term_execute(w);
        return;
    }

    if (ch == '\b') {
        if (w->term_buf_len > 0) {
            w->term_col--;
            uint32 px = w->term_col * 8;
            uint32 sy;
            for (sy = 0; sy < ch_h && sy < w->surface_h; sy++) {
                uint32 sx;
                for (sx = 0; sx < 8; sx++) {
                    w->surface[(w->term_row * ch_h + sy) * w->surface_w + px + sx] = 0;
                }
            }
            w->term_buf_len--;
            w->term_buf[w->term_buf_len] = '\0';
        }
        return;
    }

    if (w->term_buf_len < 127) {
        w->term_buf[w->term_buf_len++] = ch;
        w->term_buf[w->term_buf_len] = '\0';
    }

    term_write_char(w, ch, COL_GREEN);
}

static void init_filemanager_surface(wm_window_t *w)
{
    if (w->needs_surface_clear) clear_surface(w);
    wm_update_surface_size(w);

    const char *header = "== File Manager ==";
    uint32 i, y_off = 4 - (uint32)(fm_scroll < 0 ? 0 : fm_scroll);
    for (i = 0; header[i] && i * 8 < w->surface_w - 12; i++) {
        unsigned int idx = (unsigned int)header[i];
        if (idx >= 32 && idx <= 127) {
            idx -= 32;
            const unsigned char *g = font8x16[idx < 96 ? idx : 0];
            uint32 sy;
            for (sy = 0; sy < 14 && (int32)(sy + y_off) < (int32)w->surface_h; sy++) {
                if ((int32)(sy + y_off) < 0) continue;
                unsigned char row = g[sy < 16 ? sy : 0];
                uint32 sx;
                for (sx = 0; sx < 8; sx++) {
                    if (row & (0x80 >> sx))
                        w->surface[(sy + y_off) * w->surface_w + i * 8 + sx] = COL_BLUE;
                }
            }
        }
    }
    y_off += 20;

    for (i = 0; i < fm_entry_count; i++) {
        const char *e = fm_entries[i];
        uint32 ecol;
        if (e[1] == 'D') ecol = COL_GREEN;
        else if (e[1] == 'F') ecol = COL_YELLOW;
        else ecol = COL_WHITE;

        uint32 j;
        for (j = 0; e[j] && j * 8 < w->surface_w - 12; j++) {
            unsigned int idx2 = (unsigned int)e[j];
            if (idx2 >= 32 && idx2 <= 127) {
                idx2 -= 32;
                const unsigned char *g = font8x16[idx2 < 96 ? idx2 : 0];
                uint32 sy;
                for (sy = 0; sy < 14 && (int32)(sy + y_off) < (int32)w->surface_h; sy++) {
                    if ((int32)(sy + y_off) < 0) continue;
                    unsigned char row = g[sy < 16 ? sy : 0];
                    uint32 sx;
                    for (sx = 0; sx < 8; sx++) {
                        if (row & (0x80 >> sx))
                            w->surface[(sy + y_off) * w->surface_w + j * 8 + sx] = ecol;
                    }
                }
            }
        }
        y_off += 18;
    }

    /* Scrollbar track on the right edge */
    uint32 sb_x = w->surface_w - 10;
    uint32 sb_h = w->surface_h;
    fill_rect_in_surface(w, sb_x, 0, 8, sb_h, COL_SCROLLBAR);

    /* Content height vs viewport: only scroll if content overflows */
    uint32 content_h = 4 + 20 + fm_entry_count * 18 + 4;
    uint32 max_scroll = 0;
    if (content_h > sb_h) max_scroll = content_h - sb_h;

    if (max_scroll > 0) {
        uint32 thumb_h = (sb_h * sb_h) / content_h;
        if (thumb_h < 16) thumb_h = 16;
        uint32 scl = (uint32)(fm_scroll < 0 ? 0 : fm_scroll);
        if (scl > max_scroll) scl = max_scroll;
        uint32 thumb_y = (scl * (sb_h - thumb_h)) / max_scroll;
        if (thumb_y + thumb_h > sb_h) thumb_y = sb_h - thumb_h;
        fill_rect_in_surface(w, sb_x + 1, thumb_y, 6, thumb_h, COL_SCROLL_THUMB);
    }

    w->dirty = 1;
}

static void init_about_surface(wm_window_t *w)
{
    if (w->needs_surface_clear) clear_surface(w);
    wm_update_surface_size(w);

    const char *lines[] = {
        "RedLion OS",
        "Version 2.0.0",
        "",
        "A hobby operating system",
        "with graphical desktop",
        "",
        "Features:",
        "  - Micro-kernel arch",
        "  - Ring 3 servers",
        "  - IPC messaging",
        "  - VBE framebuffer",
        "  - PS/2 mouse",
        "",
        "Built with C, C++17,",
        "x86 assembly",
        0
    };

    uint32 y_off = 8;
    int i;
    for (i = 0; lines[i]; i++) {
        const char *line = lines[i];
        uint32 lcol = (i == 0) ? COL_YELLOW :
                      (i == 1) ? COL_BLUE : COL_WHITE;
        uint32 sc = (i == 0) ? 2 : 1;
        uint32 j;
        for (j = 0; line[j] && j * 8 * sc < w->surface_w; j++) {
            unsigned int idx = (unsigned int)line[j];
            if (idx >= 32 && idx <= 127) {
                idx -= 32;
                const unsigned char *g = font8x16[idx < 96 ? idx : 0];
                uint32 sy;
                for (sy = 0; sy < 16 * sc && sy + y_off < w->surface_h; sy++) {
                    unsigned char row = g[sy / sc < 16 ? sy / sc : 0];
                    uint32 sx;
                    for (sx = 0; sx < 8 * sc; sx++) {
                        if (row & (0x80 >> (sx / sc)))
                            w->surface[(sy + y_off) * w->surface_w + j * 8 * sc + sx] = lcol;
                    }
                }
            }
        }
        y_off += (i == 0) ? 34 : 18;
    }
    w->dirty = 1;
}

static void compose(void)
{
    draw_desktop_bg();

    int draw_order[WM_MAX_WINDOWS];
    int draw_count = 0;
    int i;

    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        if ((windows[i].flags & WM_WIN_VISIBLE) &&
            !(windows[i].flags & WM_WIN_MINIMIZED)) {
            draw_order[draw_count++] = i;
        }
    }

    {
        int a, b;
        for (a = 0; a < draw_count - 1; a++) {
            for (b = a + 1; b < draw_count; b++) {
                if (windows[draw_order[a]].z_order > windows[draw_order[b]].z_order) {
                    int t = draw_order[a];
                    draw_order[a] = draw_order[b];
                    draw_order[b] = t;
                }
            }
        }
    }

    for (i = 0; i < draw_count; i++) {
        draw_window(&windows[draw_order[i]]);
    }

    draw_taskbar();
    draw_start_menu();

    draw_cursor_at(mouse_x, mouse_y, resize_dir);
}

extern "C" void srv_wm_main(void)
{
    lfb_ptr = sys_fb_get_buffer();
    if (!lfb_ptr) return;

    screen_w = 1024;
    screen_h = 768;
    lfb_pitch = screen_w;

    int fm_id = wm_create("File Manager", 60, 50, 380, 320, WM_APP_FILE_MANAGER);
    int term_id = wm_create("Terminal", 180, 100, 420, 300, WM_APP_TERMINAL);
    int about_id = wm_create("About", 340, 160, 320, 280, WM_APP_ABOUT);

    {
        wm_window_t *w = find_window(fm_id);
        if (w) init_filemanager_surface(w);
    }
    {
        wm_window_t *w = find_window(term_id);
        if (w) init_terminal_surface(w);
    }
    {
        wm_window_t *w = find_window(about_id);
        if (w) init_about_surface(w);
    }

    focus_top();

    while (1) {
        tick_counter++;
        if (tick_counter % 50 == 0) uptime_seconds++;

        int32 mx, my, btn;
        sys_mouse_get(&mx, &my, &btn);
        prev_mouse_btn = mouse_btn;
        if (mx < 0) mx = 0;
        if (my < 0) my = 0;
        if (mx >= (int32)screen_w) mx = (int32)screen_w - 1;
        if (my >= (int32)(screen_h - 1)) my = (int32)(screen_h - 1);
        mouse_x = mx;
        mouse_y = my;
        mouse_btn = btn;

        uint32 need_redraw = 0;

        int32 scroll_delta = sys_mouse_get_scroll();
        if (scroll_delta != 0) {
            wm_window_t *fw = find_window(focused_id);
            if (fw && fw->app_type == WM_APP_FILE_MANAGER) {
                uint32 content_h = 4 + 20 + fm_entry_count * 18 + 4;
                if (content_h > fw->surface_h) {
                    int32 max_scroll = (int32)(content_h - fw->surface_h);
                    fm_scroll -= scroll_delta * 6;
                    if (fm_scroll < 0) fm_scroll = 0;
                    if (fm_scroll > max_scroll) fm_scroll = max_scroll;
                    fw->dirty = 1;
                    init_filemanager_surface(fw);
                    need_redraw = 1;
                }
            }
        }

        int32 transport_changed = 0;
        if (mouse_x != prev_mouse_x || mouse_y != prev_mouse_y || mouse_btn != prev_mouse_btn) {
            transport_changed = 1;
        }
        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;
        need_redraw = (uint32)transport_changed;

        if (resize_dir != WM_RESIZE_NONE && resize_win_idx >= 0) {
            if (mouse_btn & 1) {
                wm_window_t *w = &windows[resize_win_idx];
                int32 dx = mouse_x - resize_start_x;
                int32 dy = mouse_y - resize_start_y;

                if (resize_dir == WM_RESIZE_LEFT || resize_dir == WM_RESIZE_TL || resize_dir == WM_RESIZE_BL) {
                    int32 new_x = resize_orig_x + dx;
                    int32 new_w = (int32)resize_orig_w - dx;
                    if (new_w < 120) { new_w = 120; new_x = resize_orig_x + (int32)resize_orig_w - 120; }
                    if (new_x < 0) { new_w += new_x; new_x = 0; }
                    w->x = new_x;
                    w->w = (uint32)new_w;
                }
                if (resize_dir == WM_RESIZE_RIGHT || resize_dir == WM_RESIZE_TR || resize_dir == WM_RESIZE_BR) {
                    int32 new_w = (int32)resize_orig_w + dx;
                    if (new_w < 120) new_w = 120;
                    if (w->x + new_w > (int32)screen_w) new_w = (int32)screen_w - w->x;
                    w->w = (uint32)new_w;
                }
                if (resize_dir == WM_RESIZE_TOP || resize_dir == WM_RESIZE_TL || resize_dir == WM_RESIZE_TR) {
                    int32 new_y = resize_orig_y + dy;
                    int32 new_h = (int32)resize_orig_h - dy;
                    if (new_h < 80) { new_h = 80; new_y = resize_orig_y + (int32)resize_orig_h - 80; }
                    if (new_y < 0) { new_h += new_y; new_y = 0; }
                    w->y = new_y;
                    w->h = (uint32)new_h;
                }
                if (resize_dir == WM_RESIZE_BOTTOM || resize_dir == WM_RESIZE_BL || resize_dir == WM_RESIZE_BR) {
                    int32 new_h = (int32)resize_orig_h + dy;
                    if (new_h < 80) new_h = 80;
                    if (w->y + new_h > (int32)(screen_h - WM_TASKBAR_H))
                        new_h = (int32)(screen_h - WM_TASKBAR_H) - w->y;
                    w->h = (uint32)new_h;
                }

                w->surface_w = w->w - 2;
                w->surface_h = w->h - WM_TITLEBAR_H - 2;
                wm_update_surface_size(w);
                w->dirty = 1;
            } else {
                resize_dir = WM_RESIZE_NONE;
                resize_win_idx = -1;
            }
            compose();
            sys_fb_flush();
            continue;
        }

        if ((mouse_btn & 1) && !(prev_mouse_btn & 1)) {
            if (start_menu_open) {
                int menu_hit = start_menu_hit_test(mouse_x, mouse_y);
                start_menu_open = 0;
                if (menu_hit >= 0 && menu_hit < WM_APP_COUNT) {
                    const char *title = app_names[menu_hit];
                    int32 wx = 100 + (menu_hit * 60);
                    int32 wy = 60 + (menu_hit * 40);
                    uint32 ww = 400;
                    uint32 wh = 300;
                    if (menu_hit == WM_APP_ABOUT) { ww = 320; wh = 280; }
                    int nid = wm_create(title, wx, wy, ww, wh, (uint8)menu_hit);
                    wm_window_t *nw = find_window(nid);
                    if (nw) {
                        if (menu_hit == WM_APP_FILE_MANAGER) { fm_scroll = 0; init_filemanager_surface(nw); }
                        else if (menu_hit == WM_APP_TERMINAL) init_terminal_surface(nw);
                        else if (menu_hit == WM_APP_ABOUT) init_about_surface(nw);
                    }
                } else if (menu_hit == 100) {
                    sys_shutdown();
                } else if (menu_hit == 101) {
                    sys_reboot();
                }
                compose();
                sys_fb_flush();
                continue;
            }

            int32 tb_y = (int32)(screen_h - WM_TASKBAR_H);
            if (mouse_y >= tb_y) {
                int tb_hit = taskbar_hit_test(mouse_x, mouse_y);
                if (tb_hit == -2) {
                    start_menu_open = !start_menu_open;
                } else if (tb_hit > 0) {
                    wm_window_t *w = find_window(tb_hit);
                    if (w) {
                        if (w->flags & WM_WIN_MINIMIZED) {
                            w->flags &= ~WM_WIN_MINIMIZED;
                            w->z_order = top_z++;
                            focus_top();
                        } else if (w->focused) {
                            wm_minimize(tb_hit);
                        } else {
                            w->z_order = top_z++;
                            focus_top();
                        }
                    }
                }
                compose();
                sys_fb_flush();
                continue;
            }

            start_menu_open = 0;

            int hit = hit_test(mouse_x, mouse_y);
            if (hit >= 0) {
                wm_window_t *w = &windows[hit];

                int32 close_bx = w->x + (int32)w->w - WM_BTN_SIZE - 4;
                int32 close_by = w->y + 2;
                if (mouse_x >= close_bx && mouse_x < close_bx + WM_BTN_SIZE &&
                    mouse_y >= close_by && mouse_y < close_by + WM_BTN_SIZE) {
                    wm_destroy(w->id);
                    compose();
                    sys_fb_flush();
                    continue;
                }

                int32 max_bx = close_bx - WM_BTN_SIZE - 2;
                int32 max_by = close_by;
                if (mouse_x >= max_bx && mouse_x < max_bx + WM_BTN_SIZE &&
                    mouse_y >= max_by && mouse_y < max_by + WM_BTN_SIZE) {
                    wm_maximize(w->id);
                    w->dirty = 1;
                    compose();
                    sys_fb_flush();
                    continue;
                }

                int32 min_bx = max_bx - WM_BTN_SIZE - 2;
                int32 min_by = close_by;
                if (mouse_x >= min_bx && mouse_x < min_bx + WM_BTN_SIZE &&
                    mouse_y >= min_by && mouse_y < min_by + WM_BTN_SIZE) {
                    wm_minimize(w->id);
                    compose();
                    sys_fb_flush();
                    continue;
                }

                w->z_order = top_z++;
                focus_top();

                int32 rdir = get_resize_dir(mouse_x, mouse_y, hit);
                if (rdir != WM_RESIZE_NONE) {
                    resize_dir = rdir;
                    resize_win_idx = hit;
                    resize_start_x = mouse_x;
                    resize_start_y = mouse_y;
                    resize_orig_x = w->x;
                    resize_orig_y = w->y;
                    resize_orig_w = w->w;
                    resize_orig_h = w->h;
                    compose();
                    sys_fb_flush();
                    continue;
                }

                if (mouse_y < w->y + (int32)WM_TITLEBAR_H) {
                    int32 off_x = mouse_x - w->x;
                    int32 off_y = mouse_y - w->y;
                    while (mouse_btn & 1) {
                        int32 nmx, nmy, nbtn;
                        sys_mouse_get(&nmx, &nmy, &nbtn);
                        mouse_x = nmx;
                        mouse_y = nmy;
                        mouse_btn = nbtn;
                        w->x = mouse_x - off_x;
                        w->y = mouse_y - off_y;
                        if (w->y < 0) w->y = 0;
                        if (w->flags & WM_WIN_MAXIMIZED) {
                            w->flags &= ~WM_WIN_MAXIMIZED;
                            w->w = w->prev_w;
                            w->h = w->prev_h;
                            wm_update_surface_size(w);
                            off_x = (int32)w->w / 2;
                            off_y = 10;
                        }
                        compose();
                        sys_fb_flush();
                    }
                }
            } else {
                /* No window under cursor: try a desktop icon */
                int dhit = desktop_icon_hit_test(mouse_x, mouse_y);
                if (dhit >= 0) {
                    const char *title = app_names[dhit];
                    int32 wx = 80 + (dhit * 70);
                    int32 wy = 60 + (dhit * 50);
                    uint32 ww = 400;
                    uint32 wh = 300;
                    if (dhit == WM_APP_ABOUT) { ww = 320; wh = 280; }
                    int nid = wm_create(title, wx, wy, ww, wh, (uint8)dhit);
                    wm_window_t *nw = find_window(nid);
                    if (nw) {
                        if (dhit == WM_APP_FILE_MANAGER) { fm_scroll = 0; init_filemanager_surface(nw); }
                        else if (dhit == WM_APP_TERMINAL) init_terminal_surface(nw);
                        else if (dhit == WM_APP_ABOUT) init_about_surface(nw);
                    }
                    compose();
                    sys_fb_flush();
                    continue;
                }
            }
        }

        int32 key;
        int ktype = sys_kbd_get_event(&key);
        while (ktype != KBD_EV_NONE) {
            wm_window_t *fw = find_window(focused_id);
            if (ktype == KBD_EV_CHAR) {
                if (fw && fw->app_type == WM_APP_TERMINAL) {
                    terminal_putchar(fw, (char)key);
                    fw->dirty = 1;
                }
            } else if (ktype == KBD_EV_BACKSPACE) {
                if (fw && fw->app_type == WM_APP_TERMINAL) {
                    terminal_putchar(fw, '\b');
                    fw->dirty = 1;
                }
            } else if (ktype == KBD_EV_ENTER) {
                if (fw && fw->app_type == WM_APP_TERMINAL) {
                    terminal_putchar(fw, '\n');
                    fw->dirty = 1;
                }
            } else if (ktype == KBD_EV_TAB) {
                int ci = find_window_idx(focused_id);
                if (ci >= 0) {
                    windows[ci].z_order = top_z++;
                }
                focus_top();
            }
            need_redraw = 1;
            ktype = sys_kbd_get_event(&key);
        }

        uint32 tick_now = pit_get_ticks();
        if (tick_now >= last_clock_render + 100) {
            last_clock_render = tick_now;
            need_redraw = 1;
        }

        if (need_redraw) {
            compose();
            sys_fb_flush();
        }
    }
}
