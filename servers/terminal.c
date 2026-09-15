#include "terminal.h"
#include "../lib/syscall.h"
#include "../lib/string.h"

/* ── 8x16 bitmap font (ASCII 32-127) ──────────────────────── */
static const unsigned char term_font[96][16] = {
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

/* ── ANSI color palette (indexed 0-7) ──────────────────────── */
static const uint32 ansi_palette[8] = {
    0x00000000,  /* 0: black */
    0x000000AA,  /* 1: red */
    0x0000AA00,  /* 2: green */
    0x0000AAAA,  /* 3: yellow */
    0x00AA0000,  /* 4: blue */
    0x00AA00AA,  /* 5: magenta */
    0x0000AA88,  /* 6: cyan */
    0x00AAAAAA,  /* 7: light grey (default fg) */
};

/* ── Grid helpers ──────────────────────────────────────────── */

static void term_scroll_up(terminal_t *t)
{
    uint32 row, col;
    /* Move rows up by 1 */
    for (row = 0; row < t->rows - 1; row++) {
        for (col = 0; col < t->cols; col++) {
            t->grid[row * t->cols + col] = t->grid[(row + 1) * t->cols + col];
        }
    }
    /* Clear bottom row */
    uint32 blank = (' ' | ((uint32)0x07 << 8) | ((uint32)0x00 << 12));
    for (col = 0; col < t->cols; col++) {
        t->grid[(t->rows - 1) * t->cols + col] = blank;
    }
    t->dirty = 1;
}

static void term_newline(terminal_t *t)
{
    t->cursor_col = 0;
    t->cursor_row++;
    if (t->cursor_row >= t->rows) {
        term_scroll_up(t);
        t->cursor_row = t->rows - 1;
    }
}

static void term_put_char_at(terminal_t *t, uint32 col, uint32 row, char ch)
{
    if (col >= t->cols || row >= t->rows) return;
    uint8 fg = t->ansi_fg;
    uint8 bg = t->ansi_bg;
    if (t->ansi_bold && fg < 8) fg += 8;
    t->grid[row * t->cols + col] = (uint32)(unsigned char)ch | ((uint32)fg << 8) | ((uint32)bg << 12);
}

static void term_advance(terminal_t *t)
{
    t->cursor_col++;
    if (t->cursor_col >= t->cols) {
        term_newline(t);
    }
}

/* ── ANSI escape sequence parser ───────────────────────────── */

static void term_process_csi(terminal_t *t)
{
    uint8 *b = t->ansi_buf;
    uint32 len = t->ansi_len;
    if (len == 0) return;

    /* Parse numeric parameters (up to TERM_MAX_PARAMS) */
    uint32 params[TERM_MAX_PARAMS];
    uint32 pcount = 0;
    uint32 num = 0;
    uint32 has_num = 0;
    uint32 i;
    for (i = 0; i < len; i++) {
        if (b[i] >= '0' && b[i] <= '9') {
            num = num * 10 + (b[i] - '0');
            has_num = 1;
        } else if (b[i] == ';') {
            if (pcount < TERM_MAX_PARAMS) params[pcount++] = num;
            num = 0; has_num = 0;
        } else {
            /* Final character */
            if (has_num && pcount < TERM_MAX_PARAMS) params[pcount++] = num;

            switch (b[i]) {
            case 'H': case 'f': {
                /* Cursor position: ESC[row;colH (1-based) */
                uint32 row = pcount > 0 ? params[0] - 1 : 0;
                uint32 col = pcount > 1 ? params[1] - 1 : 0;
                if (row >= t->rows) row = t->rows - 1;
                if (col >= t->cols) col = t->cols - 1;
                t->cursor_row = row;
                t->cursor_col = col;
                break;
            }
            case 'A': {
                /* Cursor up */
                uint32 n = pcount > 0 ? params[0] : 1;
                if (t->cursor_row >= n) t->cursor_row -= n;
                else t->cursor_row = 0;
                break;
            }
            case 'B': {
                /* Cursor down */
                uint32 n = pcount > 0 ? params[0] : 1;
                t->cursor_row += n;
                if (t->cursor_row >= t->rows) t->cursor_row = t->rows - 1;
                break;
            }
            case 'C': {
                /* Cursor forward */
                uint32 n = pcount > 0 ? params[0] : 1;
                t->cursor_col += n;
                if (t->cursor_col >= t->cols) t->cursor_col = t->cols - 1;
                break;
            }
            case 'D': {
                /* Cursor back */
                uint32 n = pcount > 0 ? params[0] : 1;
                if (t->cursor_col >= n) t->cursor_col -= n;
                else t->cursor_col = 0;
                break;
            }
            case 'J': {
                /* Erase in display */
                uint32 mode = pcount > 0 ? params[0] : 0;
                if (mode == 0) {
                    /* Clear from cursor to end */
                    uint32 c, r;
                    for (r = t->cursor_row; r < t->rows; r++) {
                        uint32 sc = (r == t->cursor_row) ? t->cursor_col : 0;
                        for (c = sc; c < t->cols; c++) {
                            term_put_char_at(t, c, r, ' ');
                        }
                    }
                } else if (mode == 2) {
                    /* Clear entire screen */
                    uint32 c, r;
                    for (r = 0; r < t->rows; r++)
                        for (c = 0; c < t->cols; c++)
                            term_put_char_at(t, c, r, ' ');
                    t->cursor_row = 0;
                    t->cursor_col = 0;
                }
                break;
            }
            case 'K': {
                /* Erase in line */
                uint32 mode = pcount > 0 ? params[0] : 0;
                uint32 c;
                if (mode == 0) {
                    for (c = t->cursor_col; c < t->cols; c++)
                        term_put_char_at(t, c, t->cursor_row, ' ');
                } else if (mode == 2) {
                    for (c = 0; c < t->cols; c++)
                        term_put_char_at(t, c, t->cursor_row, ' ');
                }
                break;
            }
            case 'm': {
                /* SGR — Select Graphic Rendition */
                if (pcount == 0) {
                    /* ESC[m = reset */
                    t->ansi_fg = TERM_COLOR_DEFAULT_FG;
                    t->ansi_bg = TERM_COLOR_DEFAULT_BG;
                    t->ansi_bold = 0;
                } else {
                    uint32 j;
                    for (j = 0; j < pcount; j++) {
                        uint32 code = params[j];
                        if (code == 0) {
                            t->ansi_fg = TERM_COLOR_DEFAULT_FG;
                            t->ansi_bg = TERM_COLOR_DEFAULT_BG;
                            t->ansi_bold = 0;
                        } else if (code == 1) {
                            t->ansi_bold = 1;
                        } else if (code >= 30 && code <= 37) {
                            t->ansi_fg = (uint8)(code - 30);
                        } else if (code >= 40 && code <= 47) {
                            t->ansi_bg = (uint8)(code - 40);
                        } else if (code >= 90 && code <= 97) {
                            t->ansi_fg = (uint8)(code - 90 + 8);
                        } else if (code >= 100 && code <= 107) {
                            t->ansi_bg = (uint8)(code - 100 + 8);
                        }
                    }
                }
                break;
            }
            }
            return;
        }
    }
}

static void term_parse_byte(terminal_t *t, unsigned char byte)
{
    if (t->ansi_escaped) {
        if (byte == '[' || byte == 'O') {
            /* CSI or SS3 — start collecting params */
            t->ansi_len = 0;
            return;
        }
        if (t->ansi_len == 0) {
            /* Single-char escape (like ESC c = reset) */
            t->ansi_escaped = 0;
            return;
        }
        /* Accumulate CSI parameter bytes */
        if (t->ansi_len < sizeof(t->ansi_buf) - 1) {
            t->ansi_buf[t->ansi_len++] = byte;
        }
        /* Check if this is a final byte (letter or ~) */
        if ((byte >= '@' && byte <= '~') || byte == 'm' || byte == 'H' ||
            byte == 'J' || byte == 'K' || byte == 'A' || byte == 'B' ||
            byte == 'C' || byte == 'D' || byte == 'f' || byte == '~') {
            t->ansi_escaped = 0;
            term_process_csi(t);
        }
        return;
    }

    if (byte == 0x1B) {
        /* ESC */
        t->ansi_escaped = 1;
        t->ansi_len = 0;
        return;
    }

    if (byte == '\n' || byte == '\r') {
        term_newline(t);
        return;
    }
    if (byte == '\t') {
        uint32 spaces = 8 - (t->cursor_col % 8);
        uint32 s;
        for (s = 0; s < spaces; s++) term_advance(t);
        return;
    }
    if (byte == '\b') {
        if (t->cursor_col > 0) {
            t->cursor_col--;
            term_put_char_at(t, t->cursor_col, t->cursor_row, ' ');
        }
        return;
    }

    /* Printable character */
    term_put_char_at(t, t->cursor_col, t->cursor_row, (char)byte);
    term_advance(t);
    t->dirty = 1;
}

/* ── Public API ────────────────────────────────────────────── */

void terminal_init(terminal_t *t, uint32 pix_w, uint32 pix_h)
{
    t->pix_w = pix_w;
    t->pix_h = pix_h;
    t->cols = pix_w / 8;
    t->rows = pix_h / 16;
    t->cursor_col = 0;
    t->cursor_row = 0;
    t->cursor_visible = 1;

    t->ansi_fg = TERM_COLOR_DEFAULT_FG;
    t->ansi_bg = TERM_COLOR_DEFAULT_BG;
    t->ansi_bold = 0;
    t->ansi_len = 0;
    t->ansi_escaped = 0;

    t->line_len = 0;
    t->line_cursor = 0;
    t->history_count = 0;
    t->history_idx = -1;
    t->dirty = 1;

    /* Zero grid */
    uint32 blank = (' ' | ((uint32)0x07 << 8) | ((uint32)0x00 << 12));
    uint32 i;
    for (i = 0; i < t->cols * t->rows; i++) t->grid[i] = blank;
}

void terminal_free(terminal_t *t)
{
    /* Grid is statically allocated or mapped, nothing to free */
    (void)t;
}

void terminal_write(terminal_t *t, const char *data, uint32 len)
{
    uint32 i;
    for (i = 0; i < len; i++) {
        term_parse_byte(t, (unsigned char)data[i]);
    }
}

void terminal_writes(terminal_t *t, const char *s)
{
    while (*s) {
        term_parse_byte(t, (unsigned char)*s);
        s++;
    }
}

int terminal_key(terminal_t *t, int key_type, char ch)
{
    /* Handle special keys */
    switch (key_type) {
    case 0: /* KBD_EV_NONE */
        return 0;
    case 4: /* KBD_EV_UP */
        if (t->history_count > 0) {
            if (t->history_idx < 0) {
                /* Save current line */
                uint32 len = t->line_len < TERM_MAX_LINE - 1 ? t->line_len : TERM_MAX_LINE - 1;
                uint32 i;
                for (i = 0; i < len; i++) t->history[t->history_count % TERM_MAX_HISTORY][i] = t->line_buf[i];
                t->history[t->history_count % TERM_MAX_HISTORY][len] = '\0';
            }
            if (t->history_idx < (int32)t->history_count - 1) {
                t->history_idx++;
                uint32 hi = (t->history_count - 1 - t->history_idx) % TERM_MAX_HISTORY;
                uint32 len = 0;
                while (t->history[hi][len] && len < TERM_MAX_LINE - 1) len++;
                uint32 i;
                for (i = 0; i < len; i++) t->line_buf[i] = t->history[hi][i];
                t->line_len = len;
                t->line_cursor = len;
                t->line_buf[len] = '\0';
                /* Clear input line on screen */
                terminal_writes(t, "\r");
                uint32 c;
                for (c = 0; c < t->cols; c++) term_put_char_at(t, c, t->cursor_row, ' ');
                terminal_writes(t, "> ");
                terminal_write(t, t->line_buf, t->line_len);
            }
        }
        t->dirty = 1;
        return 0;
    case 5: /* KBD_EV_DOWN */
        if (t->history_idx >= 0) {
            t->history_idx--;
            if (t->history_idx < 0) {
                /* Restore original line */
                t->line_len = 0;
                t->line_cursor = 0;
                t->line_buf[0] = '\0';
            } else {
                uint32 hi = (t->history_count - 1 - t->history_idx) % TERM_MAX_HISTORY;
                uint32 len = 0;
                while (t->history[hi][len] && len < TERM_MAX_LINE - 1) len++;
                uint32 i;
                for (i = 0; i < len; i++) t->line_buf[i] = t->history[hi][i];
                t->line_len = len;
                t->line_cursor = len;
                t->line_buf[len] = '\0';
            }
            terminal_writes(t, "\r");
            uint32 c;
            for (c = 0; c < t->cols; c++) term_put_char_at(t, c, t->cursor_row, ' ');
            terminal_writes(t, "> ");
            terminal_write(t, t->line_buf, t->line_len);
        }
        t->dirty = 1;
        return 0;
    case 2: /* KBD_EV_LEFT */
        if (t->line_cursor > 0) t->line_cursor--;
        t->dirty = 1;
        return 0;
    case 3: /* KBD_EV_RIGHT */
        if (t->line_cursor < t->line_len) t->line_cursor++;
        t->dirty = 1;
        return 0;
    case 6: /* KBD_EV_HOME */
        t->line_cursor = 0;
        t->dirty = 1;
        return 0;
    case 7: /* KBD_EV_END */
        t->line_cursor = t->line_len;
        t->dirty = 1;
        return 0;
    case 8: /* KBD_EV_DEL */
        if (t->line_cursor < t->line_len) {
            uint32 i;
            for (i = t->line_cursor; i < t->line_len - 1; i++)
                t->line_buf[i] = t->line_buf[i + 1];
            t->line_len--;
            t->line_buf[t->line_len] = '\0';
            /* Redraw line */
            terminal_writes(t, "\r");
            uint32 c;
            for (c = 0; c < t->cols; c++) term_put_char_at(t, c, t->cursor_row, ' ');
            terminal_writes(t, "> ");
            terminal_write(t, t->line_buf, t->line_len);
        }
        t->dirty = 1;
        return 0;
    case 9: /* KBD_EV_TAB */
        /* Ignore tab for now */
        return 0;
    case 10: /* KBD_EV_BACKSPACE */
        if (t->line_len > 0 && t->line_cursor > 0) {
            uint32 i;
            for (i = t->line_cursor - 1; i < t->line_len - 1; i++)
                t->line_buf[i] = t->line_buf[i + 1];
            t->line_len--;
            t->line_cursor--;
            t->line_buf[t->line_len] = '\0';
            /* Erase character on screen */
            if (t->cursor_col > 0) {
                t->cursor_col--;
                term_put_char_at(t, t->cursor_col, t->cursor_row, ' ');
            }
        }
        t->dirty = 1;
        return 0;
    case 11: /* KBD_EV_ENTER */
        if (t->line_len > 0) {
            /* Add to history */
            uint32 hi = t->history_count % TERM_MAX_HISTORY;
            uint32 len = t->line_len < TERM_MAX_LINE - 1 ? t->line_len : TERM_MAX_LINE - 1;
            uint32 i;
            for (i = 0; i < len; i++) t->history[hi][i] = t->line_buf[i];
            t->history[hi][len] = '\0';
            t->history_count++;
            if (t->history_count > TERM_MAX_HISTORY) t->history_count = TERM_MAX_HISTORY;
        }
        t->history_idx = -1;
        term_newline(t);
        return 1; /* Enter pressed — command ready */
    }

    /* Printable character (KBD_EV_CHAR = 1) */
    if (key_type == 1 && ch >= 32 && ch < 127) {
        /* Insert at cursor position */
        if (t->line_len < TERM_MAX_LINE - 1) {
            uint32 i;
            for (i = t->line_len; i > t->line_cursor; i--)
                t->line_buf[i] = t->line_buf[i - 1];
            t->line_buf[t->line_cursor] = ch;
            t->line_len++;
            t->line_cursor++;
            t->line_buf[t->line_len] = '\0';
            /* Show character on screen */
            term_put_char_at(t, t->cursor_col, t->cursor_row, ch);
            term_advance(t);
        }
        t->dirty = 1;
    }

    return 0;
}

void terminal_render(terminal_t *t, uint32 *surface, uint32 surface_w, uint32 surface_h)
{
    uint32 row, col;
    for (row = 0; row < t->rows && row * 16 < surface_h; row++) {
        for (col = 0; col < t->cols && col * 8 < surface_w; col++) {
            uint32 cell = t->grid[row * t->cols + col];
            char ch = (char)(cell & 0xFF);
            uint8 fg = (uint8)((cell >> 8) & 0x0F);
            uint8 bg = (uint8)((cell >> 12) & 0x0F);

            uint32 fg_col = ansi_palette[fg & 7];
            if (fg & 8) {
                /* Bright: add 0x55 to each channel */
                uint32 r = (fg_col & 0xFF) + 0x55;
                uint32 g = ((fg_col >> 8) & 0xFF) + 0x55;
                uint32 b = ((fg_col >> 16) & 0xFF) + 0x55;
                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;
                fg_col = r | (g << 8) | (b << 16);
            }
            uint32 bg_col = ansi_palette[bg & 7];

            unsigned int idx = (unsigned int)ch;
            if (idx < 32 || idx > 127) idx = 32;
            idx -= 32;
            const unsigned char *glyph = term_font[idx];

            uint32 sy;
            for (sy = 0; sy < 16 && (row * 16 + sy) < surface_h; sy++) {
                unsigned char bits = glyph[sy];
                uint32 sx;
                for (sx = 0; sx < 8 && (col * 8 + sx) < surface_w; sx++) {
                    uint32 color = (bits & (0x80 >> sx)) ? fg_col : bg_col;
                    surface[(row * 16 + sy) * surface_w + col * 8 + sx] = color;
                }
            }
        }
    }

    /* Draw cursor block if visible */
    if (t->cursor_visible && t->cursor_row < t->rows && t->cursor_col < t->cols) {
        uint32 cy = t->cursor_row * 16;
        uint32 cx = t->cursor_col * 8;
        uint32 sy;
        for (sy = 14; sy < 16 && cy + sy < surface_h; sy++) {
            uint32 sx;
            for (sx = 0; sx < 8 && cx + sx < surface_w; sx++) {
                surface[(cy + sy) * surface_w + cx + sx] = 0x00AAAAAA;
            }
        }
    }

    t->dirty = 0;
}
