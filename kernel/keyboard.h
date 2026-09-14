#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

/* Special key events that are not printable characters */
#define KBD_EV_NONE             0
#define KBD_EV_CHAR             1
#define KBD_EV_LEFT             2
#define KBD_EV_RIGHT            3
#define KBD_EV_UP               4
#define KBD_EV_DOWN             5
#define KBD_EV_HOME             6
#define KBD_EV_END              7
#define KBD_EV_DEL              8
#define KBD_EV_TAB              9
#define KBD_EV_BACKSPACE        10
#define KBD_EV_ENTER            11

/* Special register values for the event system */
#define KEY_REG_NONE            0
#define KEY_REG_LEFTSHIFT       1
#define KEY_REG_RIGHTSHIFT      2
#define KEY_REG_CAPSLOCKED      4
#define KEY_REG_SHIFT           3

/** keyboard_init:
 *  Initializes the keyboard driver.
 */
void keyboard_init(void);

/** keyboard_handle_interrupt:
 *  Handles a keyboard interrupt and converts a scan code into a key event.
 *
 *  @param scan_code The scan code from the keyboard
 */
void keyboard_handle_interrupt(unsigned char scan_code);

/** keyboard_get_event:
 *  Gets the next pending key event (non-blocking).
 *
 *  @param ch  Output: the character if the event is KBD_EV_CHAR
 *  @return    The event type (KBD_EV_*), or KBD_EV_NONE
 */
int keyboard_get_event(char *ch);

/** keyboard_get_char:
 *  Gets the next printable character typed (non-blocking).
 *
 *  @return The character, or 0 if no character available
 */
char keyboard_get_char(void);

#endif /* INCLUDE_KEYBOARD_H */