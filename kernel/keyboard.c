#include "keyboard.h"

/* US QWERTY keyboard layout, unshifted scan code to ASCII table.
 * Indexed directly by scan code. */
static char scan_code_base[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* Shifted layout */
static char scan_code_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

/* Modifier scan codes */
#define KEY_SC_LEFTSHIFT_DOWN   0x2A
#define KEY_SC_RIGHTSHIFT_DOWN  0x36
#define KEY_SC_LEFTSHIFT_UP     0xAA
#define KEY_SC_RIGHTSHIFT_UP    0xB6
#define KEY_SC_CAPSLOCK         0x3A

/* Event queue */
#define KBD_EVENT_QUEUE_SIZE 64

struct kbd_event {
    int type;
    char ch;
};

static struct kbd_event event_queue[KBD_EVENT_QUEUE_SIZE];
static unsigned int queue_head = 0;
static unsigned int queue_tail = 0;

static char shift_state = 0;
static char caps_state = 0;
static char extended = 0;

/** kbd_queue_push:
 *  Pushes an event into the queue, dropping the oldest if full.
 */
static void kbd_queue_push(int type, char ch)
{
    unsigned int next;

    next = (queue_tail + 1) % KBD_EVENT_QUEUE_SIZE;
    if (next == queue_head) {
        queue_head = (queue_head + 1) % KBD_EVENT_QUEUE_SIZE;
    }
    event_queue[queue_tail].type = type;
    event_queue[queue_tail].ch = ch;
    queue_tail = next;
}

/** kbd_queue_pop:
 *  Pops the next event, or returns KBD_EV_NONE if empty.
 */
static int kbd_queue_pop(char *ch)
{
    int type;

    if (queue_head == queue_tail) {
        return KBD_EV_NONE;
    }
    type = event_queue[queue_head].type;
    if (ch != 0) {
        *ch = event_queue[queue_head].ch;
    }
    queue_head = (queue_head + 1) % KBD_EVENT_QUEUE_SIZE;
    return type;
}

/** keyboard_init:
 *  Initializes the keyboard driver.
 */
void keyboard_init(void)
{
    shift_state = 0;
    caps_state = 0;
    extended = 0;
    queue_head = 0;
    queue_tail = 0;
}

/** keyboard_handle_interrupt:
 *  Handles a keyboard interrupt and converts a scan code into a key event.
 *
 *  @param scan_code The scan code from the keyboard
 */
void keyboard_handle_interrupt(unsigned char scan_code)
{
    char c;

    /* Extended scan code prefix */
    if (scan_code == 0xE0) {
        extended = 1;
        return;
    }

    if (extended) {
        extended = 0;
        switch (scan_code) {
            case 0x4B: kbd_queue_push(KBD_EV_LEFT, 0); return;
            case 0x4D: kbd_queue_push(KBD_EV_RIGHT, 0); return;
            case 0x48: kbd_queue_push(KBD_EV_UP, 0); return;
            case 0x50: kbd_queue_push(KBD_EV_DOWN, 0); return;
            case 0x47: kbd_queue_push(KBD_EV_HOME, 0); return;
            case 0x4F: kbd_queue_push(KBD_EV_END, 0); return;
            case 0x53: kbd_queue_push(KBD_EV_DEL, 0); return;
            default: return;
        }
    }

    /* Key releases */
    if (scan_code >= 0x80) {
        switch (scan_code) {
            case KEY_SC_LEFTSHIFT_UP:
            case KEY_SC_RIGHTSHIFT_UP:
                shift_state = 0;
                break;
            default:
                break;
        }
        return;
    }

    /* Key presses */
    switch (scan_code) {
        case KEY_SC_LEFTSHIFT_DOWN:
        case KEY_SC_RIGHTSHIFT_DOWN:
            shift_state = 1;
            return;
        case KEY_SC_CAPSLOCK:
            caps_state = !caps_state;
            return;
        default:
            break;
    }

    /* Non-extended special keys */
    switch (scan_code) {
        case 0x47: kbd_queue_push(KBD_EV_HOME, 0); return;
        case 0x4F: kbd_queue_push(KBD_EV_END, 0); return;
        case 0x48: kbd_queue_push(KBD_EV_UP, 0); return;
        case 0x50: kbd_queue_push(KBD_EV_DOWN, 0); return;
        case 0x4B: kbd_queue_push(KBD_EV_LEFT, 0); return;
        case 0x4D: kbd_queue_push(KBD_EV_RIGHT, 0); return;
        case 0x53: kbd_queue_push(KBD_EV_DEL, 0); return;
        default: break;
    }

    /* Printable character */
    if (scan_code <= 0x39) {
        c = shift_state ? scan_code_shift[scan_code] : scan_code_base[scan_code];

        /* CapsLock inverts the case of letters */
        if (c >= 'a' && c <= 'z' && caps_state) {
            c -= 32;
        } else if (c >= 'A' && c <= 'Z' && caps_state) {
            c += 32;
        }

        if (c == '\t') {
            if (extended) {
                extended = 0;
            }
            kbd_queue_push(KBD_EV_TAB, 0);
            return;
        }
        if (c == '\b') {
            if (extended) {
                extended = 0;
            }
            kbd_queue_push(KBD_EV_BACKSPACE, 0);
            return;
        }
        if (c == '\n') {
            kbd_queue_push(KBD_EV_ENTER, 0);
            return;
        }
        if (c != 0) {
            kbd_queue_push(KBD_EV_CHAR, c);
        }
        return;
    }

    if (scan_code == 0x0F) {
        kbd_queue_push(KBD_EV_TAB, 0);
    }
}

/** keyboard_get_event:
 *  Gets the next pending key event (non-blocking).
 *
 *  @param ch  Output: the character if the event is KBD_EV_CHAR
 *  @return    The event type (KBD_EV_*), or KBD_EV_NONE
 */
int keyboard_get_event(char *ch)
{
    return kbd_queue_pop(ch);
}

/** keyboard_get_char:
 *  Gets the next printable character typed (non-blocking).
 *
 *  @return The character, or 0 if no character available
 */
char keyboard_get_char(void)
{
    char ch = 0;
    int type;
    int count = KBD_EVENT_QUEUE_SIZE;

    while (count-- > 0) {
        type = kbd_queue_pop(&ch);
        if (type == KBD_EV_CHAR) {
            return ch;
        }
        if (type == KBD_EV_NONE) {
            return 0;
        }
    }
    return 0;
}