#include "lineedit.h"
#include "fb.h"
#include "keyboard.h"
#include "string.h"

static int le_line_start;
static int le_len;
static int le_cursor;
static int le_hist_count;
static int le_hist_idx;
static int le_hist_cap;
static char (*le_history)[256];
static char le_save[256];
static int le_prompt_len;
static char *le_buf;
static int   le_buf_size;

/* Maximum chars we can display on screen after the prompt */
static int le_max_visible(void)
{
    return fb_width() * fb_height() - le_line_start;
}

/** le_redraw:
 *  Redraws the current edit line on screen from the known start position.
 */
static void le_redraw(void)
{
    int i;
    int vis = le_max_visible();
    int show_len = le_len < vis ? le_len : vis;

    for (i = 0; i < show_len; i++) {
        fb_write_cell((le_line_start + i) * 2, le_buf[i], FB_WHITE, FB_BLACK);
    }
    for (i = show_len; i < vis; i++) {
        fb_write_cell((le_line_start + i) * 2, ' ', FB_WHITE, FB_BLACK);
    }
    fb_move_cursor((unsigned short)(le_line_start + le_cursor));
}

/** lineedit_init:
 *  Initializes the line editor.
 */
void lineedit_init(int prompt_len, char history_buf[][256])
{
    le_line_start = fb_get_cursor_pos();
    le_len = 0;
    le_cursor = 0;
    le_hist_count = 0;
    le_hist_idx = -1;
    le_hist_cap = LINEEDIT_HISTORY_SIZE;
    le_history = history_buf;
    le_prompt_len = prompt_len;
    memset(le_save, 0, 256);
}

/** lineedit_update:
 *  Processes a key event and updates the display.
 *  Call this in the main loop.
 *
 *  @param buf       The working line buffer (written by the editor)
 *  @param buf_size  Size of buf
 *  @return          1 when the user pressed Enter (line is in buf), 0 otherwise
 */
int lineedit_update(char *buf, int buf_size)
{
    char ch;
    int ev;
    int i;

    le_buf = buf;
    le_buf_size = buf_size;
    ev = keyboard_get_event(&ch);

    if (ev == KBD_EV_NONE) {
        return 0;
    }

    if (ev == KBD_EV_CHAR) {
        if (le_len < le_buf_size - 1) {
            /* Shift right */
            for (i = le_len; i > le_cursor; i--) {
                le_buf[i] = le_buf[i - 1];
            }
            le_buf[le_cursor] = ch;
            le_len++;
            le_cursor++;
            le_buf[le_len] = '\0';
            le_redraw();
        }
        return 0;
    }

    if (ev == KBD_EV_BACKSPACE) {
        if (le_cursor > 0) {
            for (i = le_cursor - 1; i < le_len - 1; i++) {
                le_buf[i] = le_buf[i + 1];
            }
            le_len--;
            le_cursor--;
            le_buf[le_len] = '\0';
            le_redraw();
        }
        return 0;
    }

    if (ev == KBD_EV_DEL) {
        if (le_cursor < le_len) {
            for (i = le_cursor; i < le_len - 1; i++) {
                le_buf[i] = le_buf[i + 1];
            }
            le_len--;
            le_buf[le_len] = '\0';
            le_redraw();
        }
        return 0;
    }

    if (ev == KBD_EV_LEFT) {
        if (le_cursor > 0) {
            le_cursor--;
            le_redraw();
        }
        return 0;
    }

    if (ev == KBD_EV_RIGHT) {
        if (le_cursor < le_len) {
            le_cursor++;
            le_redraw();
        }
        return 0;
    }

    if (ev == KBD_EV_HOME) {
        le_cursor = 0;
        le_redraw();
        return 0;
    }

    if (ev == KBD_EV_END) {
        le_cursor = le_len;
        le_redraw();
        return 0;
    }

    if (ev == KBD_EV_UP) {
        if (le_hist_count == 0) {
            return 0;
        }
        /* Save current line if first time pressing up */
        if (le_hist_idx < 0) {
            strcpy(le_save, le_buf);
        }
        le_hist_idx++;
        if (le_hist_idx >= le_hist_count) {
            le_hist_idx = le_hist_count - 1;
        }
        strcpy(le_buf, le_history[le_hist_count - 1 - le_hist_idx]);
        le_len = (int)strlen(le_buf);
        le_cursor = le_len;
        le_redraw();
        return 0;
    }

    if (ev == KBD_EV_DOWN) {
        if (le_hist_idx < 0) {
            return 0;
        }
        le_hist_idx--;
        if (le_hist_idx < 0) {
            le_hist_idx = -1;
            strcpy(le_buf, le_save);
        } else {
            strcpy(le_buf, le_history[le_hist_count - 1 - le_hist_idx]);
        }
        le_len = (int)strlen(le_buf);
        le_cursor = le_len;
        le_redraw();
        return 0;
    }

    if (ev == KBD_EV_ENTER) {
        return 1;
    }

    return 0;
}

/** lineedit_history_add:
 *  Adds a line to the history.
 */
void lineedit_history_add(const char *line, char history_buf[][256])
{
    int len;
    int i;

    if (line[0] == '\0') {
        return;
    }

    /* Don't add duplicates of the last entry */
    if (le_hist_count > 0 && strcmp(history_buf[le_hist_count - 1], line) == 0) {
        return;
    }

    if (le_hist_count >= LINEEDIT_HISTORY_SIZE) {
        /* Shift entries down, dropping the oldest */
        for (i = 0; i < LINEEDIT_HISTORY_SIZE - 1; i++) {
            strcpy(history_buf[i], history_buf[i + 1]);
        }
        le_hist_count = LINEEDIT_HISTORY_SIZE - 1;
    }

    len = (int)strlen(line);
    if (len > 255) {
        len = 255;
    }
    strncpy(history_buf[le_hist_count], line, 255);
    history_buf[le_hist_count][255] = '\0';
    le_hist_count++;
}