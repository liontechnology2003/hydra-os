#ifndef SERVERS_TERMINAL_H
#define SERVERS_TERMINAL_H

#include "types.h"

#define TERM_MAX_LINE   256
#define TERM_MAX_HISTORY 32
#define TERM_MAX_PARAMS  8

/* ANSI default colors */
#define TERM_COLOR_DEFAULT_FG  7   /* light grey */
#define TERM_COLOR_DEFAULT_BG  0   /* black */

typedef struct {
    /* Text grid — static buffer, max 160 cols x 60 rows */
    uint32 grid[160 * 60];
    uint32 cols, rows;
    uint32 cursor_col, cursor_row;
    uint8  cursor_visible;

    /* Screen dimensions in pixels */
    uint32 pix_w, pix_h;

    /* ANSI parse state */
    uint8  ansi_fg;
    uint8  ansi_bg;
    uint8  ansi_bold;
    uint8  ansi_buf[32];
    uint32 ansi_len;
    uint8  ansi_escaped;

    /* Input line */
    char   line_buf[TERM_MAX_LINE];
    uint32 line_len;
    uint32 line_cursor;

    /* Command history */
    char   history[TERM_MAX_HISTORY][TERM_MAX_LINE];
    uint32 history_count;
    int32  history_idx;     /* -1 = editing current line */

    /* Dirty flag — caller checks and re-renders */
    uint8  dirty;
} terminal_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize terminal with pixel dimensions */
void terminal_init(terminal_t *t, uint32 pix_w, uint32 pix_h);

/* Free terminal resources */
void terminal_free(terminal_t *t);

/* Write raw data (handles ANSI parsing) */
void terminal_write(terminal_t *t, const char *data, uint32 len);

/* Write a C string */
void terminal_writes(terminal_t *t, const char *s);

/* Handle a keyboard event (returns 1 if Enter was pressed with a command) */
int terminal_key(terminal_t *t, int key_type, char ch);

/* Render the text grid into a pixel buffer (surface) */
void terminal_render(terminal_t *t, uint32 *surface, uint32 surface_w, uint32 surface_h);

#ifdef __cplusplus
}
#endif

#endif /* SERVERS_TERMINAL_H */
