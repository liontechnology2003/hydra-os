#ifndef SERVERS_WM_H
#define SERVERS_WM_H

#include "types.h"
#include "terminal.h"

#define WM_MAX_WINDOWS    16
#define WM_MAX_TITLE      64
#define WM_MAX_SURFACE    (480 * 300)
#define WM_MAX_SURFACE_W  480
#define WM_MAX_SURFACE_H  300
#define WM_MAX_NAME       32

#define WM_WIN_NONE       0x00
#define WM_WIN_VISIBLE    0x01
#define WM_WIN_DECORATED  0x02
#define WM_WIN_MINIMIZED  0x04
#define WM_WIN_MAXIMIZED  0x08

#define WM_TITLEBAR_H     30
#define WM_BORDER_W       2
#define WM_TASKBAR_H      36
#define WM_CORNER_R       4
#define WM_BTN_SIZE       16
#define WM_SHADOW_OFF     4
#define WM_RESIZE_ZONE    6

#define WM_RESIZE_NONE    0
#define WM_RESIZE_LEFT    1
#define WM_RESIZE_RIGHT   2
#define WM_RESIZE_TOP     3
#define WM_RESIZE_BOTTOM  4
#define WM_RESIZE_TL      5
#define WM_RESIZE_TR      6
#define WM_RESIZE_BL      7
#define WM_RESIZE_BR      8

#define WM_APP_FILE_MANAGER 0
#define WM_APP_TERMINAL     1
#define WM_APP_ABOUT        2
#define WM_APP_COUNT        3

typedef struct {
    uint32 id;
    char title[WM_MAX_TITLE];
    int32 x, y;
    uint32 w, h;
    int32 prev_x, prev_y;
    uint32 prev_w, prev_h;
    uint32 flags;
    int32 z_order;
    uint32 surface[WM_MAX_SURFACE];
    uint32 surface_w, surface_h;
    uint8  focused;
    uint8  dirty;
    uint8  app_type;
    uint8  needs_surface_clear;
    terminal_t *term;  /* non-NULL for terminal windows */
} wm_window_t;

#ifdef __cplusplus
extern "C" {
#endif
void srv_wm_main(void);
#ifdef __cplusplus
}
#endif

#endif
