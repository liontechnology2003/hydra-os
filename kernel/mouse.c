#include "mouse.h"
#include "io.h"

/* PS/2 ports */
#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

/* PS/2 commands */
#define PS2_CMD_ENABLE_AUX  0xA8
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_SELF_TEST   0xAA

/* Mouse commands (sent via data port after enabling) */
#define MOUSE_CMD_RESET     0xFF
#define MOUSE_CMD_ENABLE    0xF4
#define MOUSE_CMD_SET_RATE  0xF3
#define MOUSE_CMD_GET_ID    0xF2

/* Mouse state machine (3/4-byte packet) */
static unsigned char mouse_cycle = 0;
static unsigned char mouse_intelli = 0;   /* 1 = 4-byte IntelliMouse with wheel */
static signed char mouse_x = 0;
static signed char mouse_y = 0;
static unsigned char mouse_buttons = 0;
static signed char mouse_scroll = 0;

/* Current absolute position */
static int abs_x = 512;
static int abs_y = 384;

/* Event queue */
#define MOUSE_EVENT_QUEUE_SIZE 32
static mouse_event_t event_queue[MOUSE_EVENT_QUEUE_SIZE];
static unsigned int queue_head = 0;
static unsigned int queue_tail = 0;

static void mouse_queue_event(int type, int dx, int dy, int buttons, int scroll)
{
    unsigned int next = (queue_tail + 1) % MOUSE_EVENT_QUEUE_SIZE;
    if (next == queue_head) {
        queue_head = (queue_head + 1) % MOUSE_EVENT_QUEUE_SIZE;
    }
    event_queue[queue_tail].type = type;
    event_queue[queue_tail].dx = dx;
    event_queue[queue_tail].dy = dy;
    event_queue[queue_tail].x = abs_x;
    event_queue[queue_tail].y = abs_y;
    event_queue[queue_tail].buttons = buttons;
    event_queue[queue_tail].scroll = scroll;
    queue_tail = next;
}

static void mouse_wait_input(void)
{
    unsigned int timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & 0x01) == 0 && timeout-- > 0);
}

static void mouse_wait_output(void)
{
    unsigned int timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & 0x02) != 0 && timeout-- > 0);
}

static void mouse_write(unsigned char data)
{
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, 0xD4);
    mouse_wait_output();
    outb(PS2_DATA_PORT, data);
}

static unsigned char mouse_read(void)
{
    mouse_wait_input();
    return inb(PS2_DATA_PORT);
}

/* Process a complete PS/2 mouse packet (b0=status, b1=X, b2=Y, b3=scroll if wheel). */
static void mouse_process_packet(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3)
{
    mouse_buttons = b0 & 0x07;
    {
        signed char dx = (signed char)b1;
        signed char dy = -(signed char)b2;  /* Invert Y: screen Y goes down */

        mouse_x = dx;
        mouse_y = dy;

        /* Update absolute position */
        abs_x += dx;
        abs_y += dy;

        /* Clamp to screen bounds */
        if (abs_x < 0) abs_x = 0;
        if (abs_y < 0) abs_y = 0;
        if (abs_x > 1023) abs_x = 1023;
        if (abs_y > 767) abs_y = 767;
    }

    /* Scroll wheel: only valid on 4-byte IntelliMouse packets.
     * Byte 4 lower 4 bits: 0x01-0x07 = scroll down, 0x09-0x0F = scroll up. */
    if (mouse_intelli) {
        unsigned char s = b3 & 0x0F;
        mouse_scroll = (s >= 8) ? (signed char)(s - 16) : (signed char)s;
    } else {
        mouse_scroll = 0;
    }

    /* Queue movement event if there's movement */
    if (mouse_x != 0 || mouse_y != 0) {
        mouse_queue_event(MOUSE_EV_MOVE, mouse_x, mouse_y, mouse_buttons, 0);
    }

    /* Queue click/release events */
    static unsigned char prev_buttons = 0;
    if ((mouse_buttons & 1) && !(prev_buttons & 1)) {
        mouse_queue_event(MOUSE_EV_CLICK, 0, 0, 1, 0);
    }
    if (!(mouse_buttons & 1) && (prev_buttons & 1)) {
        mouse_queue_event(MOUSE_EV_RELEASE, 0, 0, 1, 0);
    }
    if ((mouse_buttons & 2) && !(prev_buttons & 2)) {
        mouse_queue_event(MOUSE_EV_CLICK, 0, 0, 2, 0);
    }
    if (!(mouse_buttons & 2) && (prev_buttons & 2)) {
        mouse_queue_event(MOUSE_EV_RELEASE, 0, 0, 2, 0);
    }

    prev_buttons = mouse_buttons;

    /* Queue scroll event */
    if (mouse_scroll != 0) {
        mouse_queue_event(MOUSE_EV_SCROLL, 0, 0, mouse_buttons, mouse_scroll);
    }

    mouse_cycle = 0;
}

void mouse_init(void)
{
    queue_head = 0;
    queue_tail = 0;
    mouse_cycle = 0;
    mouse_intelli = 0;
    abs_x = 512;
    abs_y = 384;
    mouse_buttons = 0;
    mouse_scroll = 0;

    /* Mask IRQ12 on the slave PIC so interrupt-driven reads don't steal
     * bytes from the poll-based init protocol below. IRQ12 = slave bit 4. */
    outb(0xA1, inb(0xA1) | 0x10);

    /* Enable auxiliary device */
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_AUX);

    /* Read and modify config byte */
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);
    mouse_wait_input();
    unsigned char config = inb(PS2_DATA_PORT);
    config |= 0x02;   /* Enable IRQ12 (aux) */
    config &= ~0x20;  /* Enable mouse */
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    mouse_wait_output();
    outb(PS2_DATA_PORT, config);

    /* Reset mouse. It responds with: 0xFA (ACK), 0xAA (BAT self-test
     * passed), then 0x00 (device ID). Consume all three so the
     * negotiation below stays byte-aligned. */
    mouse_write(MOUSE_CMD_RESET);
    (void)mouse_read(); /* 0xFA ACK */
    (void)mouse_read(); /* 0xAA BAT self-test passed */
    (void)mouse_read(); /* 0x00 device ID */

    /* Try to enable IntelliMouse (scroll wheel) mode:
     * the magic 200 -> 100 -> 80 sample-rate sequence makes the
     * controller report 4-byte packets (device ID 3). If the mouse
     * doesn't support it, GET_ID still returns 0 and we stay in 3-byte mode. */
    {
        mouse_write(MOUSE_CMD_SET_RATE);
        (void)mouse_read(); /* ACK */
        mouse_write(200);
        (void)mouse_read(); /* ACK */

        mouse_write(MOUSE_CMD_SET_RATE);
        (void)mouse_read(); /* ACK */
        mouse_write(100);
        (void)mouse_read(); /* ACK */

        mouse_write(MOUSE_CMD_SET_RATE);
        (void)mouse_read(); /* ACK */
        mouse_write(80);
        (void)mouse_read(); /* ACK */

        mouse_write(MOUSE_CMD_GET_ID);
        (void)mouse_read(); /* ACK */
        unsigned char dev_id = mouse_read(); /* device ID: 3 = wheel */
        mouse_intelli = (dev_id == 3) ? 1 : 0;
    }

    /* Set sample rate 100Hz */
    mouse_write(MOUSE_CMD_SET_RATE);
    (void)mouse_read(); /* ACK */
    mouse_write(100);
    (void)mouse_read(); /* ACK */

    /* Enable data reporting */
    mouse_write(MOUSE_CMD_ENABLE);
    (void)mouse_read(); /* ACK */

    /* Drain any residual bytes queued during init (e.g. the 0xAA
     * self-test result that follows the RESET ACK) so the first IRQ
     * after unmasking starts a clean packet, not a stale byte. */
    {
        unsigned int guard = 100;
        while (guard-- > 0 && (inb(PS2_STATUS_PORT) & 0x01) != 0) {
            (void)inb(PS2_DATA_PORT);
        }
    }

    /* Unmask IRQ12 after init completes */
    outb(0xA1, inb(0xA1) & ~0x10);
}

/* Buffered 4-byte packet while the state machine assembles it. */
static unsigned char packet_buf[4];

void mouse_handle_interrupt(void)
{
    unsigned char packet = inb(PS2_DATA_PORT);
    unsigned char last_byte = mouse_intelli ? 3 : 2;

    if (mouse_cycle < last_byte) {
        packet_buf[mouse_cycle] = packet;
        mouse_cycle++;
    } else {
        packet_buf[last_byte] = packet;
        mouse_process_packet(packet_buf[0], packet_buf[1], packet_buf[2],
                             mouse_intelli ? packet_buf[3] : 0);
        mouse_cycle = 0;
    }
}

int mouse_get_event(mouse_event_t *ev)
{
    if (queue_head == queue_tail) return -1;
    *ev = event_queue[queue_head];
    queue_head = (queue_head + 1) % MOUSE_EVENT_QUEUE_SIZE;
    return 0;
}

int mouse_get_x(void) { return abs_x; }
int mouse_get_y(void) { return abs_y; }
int mouse_get_buttons(void) { return mouse_buttons; }
int mouse_get_scroll(void) { int s = mouse_scroll; mouse_scroll = 0; return s; }
