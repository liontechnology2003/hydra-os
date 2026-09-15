#ifndef KERNEL_THEME_H
#define KERNEL_THEME_H

#include "gfx.h"

/* ── Window chrome ─────────────────────────────────────────── */
#define THEME_WIN_BG            GFX_COLOR(0x1E, 0x1E, 0x2E)
#define THEME_TITLEBAR_ACTIVE   GFX_COLOR(0x2A, 0x2A, 0x4A)
#define THEME_TITLEBAR_INACTIVE GFX_COLOR(0x25, 0x25, 0x35)
#define THEME_TITLE_ACTIVE      GFX_COLOR(0xFF, 0xFF, 0xFF)
#define THEME_TITLE_INACTIVE    GFX_COLOR(0x88, 0x88, 0x99)
#define THEME_BORDER_ACTIVE     GFX_COLOR(0xCC, 0x88, 0x44)
#define THEME_BORDER_INACTIVE   GFX_COLOR(0x44, 0x44, 0x55)
#define THEME_CLIENT_BG         GFX_COLOR(0x12, 0x12, 0x1E)
#define THEME_SHADOW            GFX_COLOR(0x00, 0x00, 0x00)

/* ── Accent / text ─────────────────────────────────────────── */
#define THEME_ACCENT            GFX_COLOR(0xCC, 0x88, 0x44)
#define THEME_TEXT              GFX_COLOR(0xCC, 0xCC, 0xDD)
#define THEME_TEXT_DIM          GFX_COLOR(0x66, 0x66, 0x88)
#define THEME_TEXT_BRIGHT       GFX_COLOR(0xBB, 0xBB, 0xCC)

/* ── Taskbar ───────────────────────────────────────────────── */
#define THEME_TASKBAR_BG        GFX_COLOR(0x16, 0x16, 0x26)
#define THEME_TASKBAR_BTN       GFX_COLOR(0x2A, 0x2A, 0x4A)
#define THEME_TASKBAR_ACTIVE    GFX_COLOR(0xCC, 0x88, 0x44)
#define THEME_TASKBAR_TEXT      GFX_COLOR(0xBB, 0xBB, 0xCC)

/* ── Menu ──────────────────────────────────────────────────── */
#define THEME_MENU_BG           GFX_COLOR(0x1E, 0x1E, 0x2E)
#define THEME_MENU_HOVER        GFX_COLOR(0x33, 0x44, 0x66)
#define THEME_MENU_SEP          GFX_COLOR(0x44, 0x44, 0x55)
#define THEME_MENU_TEXT         GFX_COLOR(0xCC, 0xCC, 0xDD)
#define THEME_MENU_HEADING      GFX_COLOR(0xCC, 0x88, 0x44)

/* ── Buttons ───────────────────────────────────────────────── */
#define THEME_BTN_CLOSE         GFX_COLOR(0xDD, 0x44, 0x44)
#define THEME_BTN_CLOSE_HOVER   GFX_COLOR(0xFF, 0x55, 0x55)
#define THEME_BTN_MINIMIZE      GFX_COLOR(0xDD, 0xAA, 0x33)
#define THEME_BTN_MINIMIZE_HOVER GFX_COLOR(0xFF, 0xCC, 0x55)
#define THEME_BTN_MAXIMIZE      GFX_COLOR(0x44, 0xBB, 0x66)
#define THEME_BTN_MAXIMIZE_HOVER GFX_COLOR(0x55, 0xDD, 0x88)

/* ── Desktop ───────────────────────────────────────────────── */
#define THEME_DESKTOP_TOP       GFX_COLOR(0x16, 0x20, 0x30)
#define THEME_DESKTOP_BOT       GFX_COLOR(0x0E, 0x14, 0x20)
#define THEME_ICON_BG           GFX_COLOR(0x22, 0x28, 0x40)
#define THEME_ICON_HOVER        GFX_COLOR(0x33, 0x40, 0x60)

/* ── Scrollbar ─────────────────────────────────────────────── */
#define THEME_SCROLLBAR_BG      GFX_COLOR(0x33, 0x33, 0x44)
#define THEME_SCROLLBAR_THUMB   GFX_COLOR(0x55, 0x55, 0x66)

/* ── App dot colors (taskbar / icons) ──────────────────────── */
#define THEME_DOT_GREEN         GFX_COLOR(0x44, 0xCC, 0x44)
#define THEME_DOT_BLUE          GFX_COLOR(0xCC, 0x88, 0x44)
#define THEME_DOT_YELLOW        GFX_COLOR(0xCC, 0xCC, 0x44)

/* ── Layout constants ──────────────────────────────────────── */
#define THEME_TITLEBAR_H        30
#define THEME_CORNER_R          4
#define THEME_PADDING           6
#define THEME_BORDER_W          2
#define THEME_BTN_SIZE          16
#define THEME_SHADOW_OFF        4
#define THEME_RESIZE_ZONE       6
#define THEME_TASKBAR_H         36

#endif /* KERNEL_THEME_H */
