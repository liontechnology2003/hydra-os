#ifndef KERNEL_MOUSE_H
#define KERNEL_MOUSE_H

#include "types.h"

/* Mouse event types */
#define MOUSE_EV_MOVE    1
#define MOUSE_EV_CLICK   2
#define MOUSE_EV_RELEASE 3
#define MOUSE_EV_SCROLL  4

typedef struct {
    int type;
    int dx, dy;          /* delta movement */
    int x, y;            /* absolute position */
    int buttons;         /* bit 0=left, bit 1=right, bit 2=middle */
    int scroll;          /* scroll delta (+/-) */
} mouse_event_t;

/* Initialize PS/2 mouse (enables aux device, sets stream mode) */
void mouse_init(void);

/* Handle mouse IRQ (IRQ12) - call from interrupt handler */
void mouse_handle_interrupt(void);

/* Get next mouse event (non-blocking, returns 0 on success) */
int mouse_get_event(mouse_event_t *ev);

/* Get current absolute position */
int mouse_get_x(void);
int mouse_get_y(void);
int mouse_get_buttons(void);
int mouse_get_scroll(void);

#endif /* KERNEL_MOUSE_H */
