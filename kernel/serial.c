#include "io.h"
#include "serial.h"

/** serial_configure_baud_rate:
 *  Sets the speed of the data being sent. The default speed of a serial
 *  port is 115200 bits/s. The argument is a divisor of that number, hence
 *  the resulting speed becomes (115200 / divisor) bits/s.
 *
 *  @param com      The COM port to configure
 *  @param divisor  The divisor
 */
void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com),
         SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com),
         (divisor >> 8) & 0x00FF);
    outb(SERIAL_DATA_PORT(com),
         divisor & 0x00FF);
}

/** serial_configure_line:
 *  Configures the line of the given serial port. The port is set to have a
 *  data length of 8 bits, no parity bits, one stop bit and break control
 *  disabled.
 *
 *  @param com  The serial port to configure
 */
void serial_configure_line(unsigned short com)
{
    /* Bit:     | 7 | 6 | 5 4 3 | 2 | 1 0 |
     * Content: | d | b | prty  | s | dl  |
     * Value:   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
     */
    outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

/** serial_configure_fifo_buffer:
 *  Configures the FIFO buffer of the given serial port.
 *
 *  @param com  The serial port to configure
 */
void serial_configure_fifo_buffer(unsigned short com)
{
    /* Bit:     | 7 6 | 5  | 4 | 3   | 2   | 1   | 0 |
     * Content: | lvl | bs | r | dma | clt | clr | e |
     * Value:   | 1 1 | 0  | 0 | 0   | 1   | 1   | 1 | = 0xC7
     */
    outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

/** serial_configure_modem:
 *  Configures the modem of the given serial port.
 *
 *  @param com  The serial port to configure
 */
void serial_configure_modem(unsigned short com)
{
    /* Bit:     | 7 | 6 | 5  | 4  | 3   | 2   | 1   | 0   |
     * Content: | r | r | af | lb | ao2 | ao1 | rts | dtr |
     * Value:   | 0 | 0 | 0  | 0  | 0   | 0   | 1   | 1   | = 0x03
     */
    outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

/** serial_is_transmit_fifo_empty:
 *  Checks whether the transmit FIFO queue is empty or not for the given COM
 *  port.
 *
 *  @param  com The COM port
 *  @return 0 if the transmit FIFO queue is not empty
 *          1 if the transmit FIFO queue is empty
 */
int serial_is_transmit_fifo_empty(unsigned int com)
{
    /* 0x20 = 0010 0000 */
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

/** serial_write:
 *  Writes the contents of the buffer buf of length len to the serial port.
 *
 *  @param buf  The buffer to write
 *  @param len  The length of the buffer
 *  @return     The number of bytes written
 */
int serial_write(const char *buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        while (serial_is_transmit_fifo_empty(SERIAL_COM1_BASE) == 0);
        outb(SERIAL_DATA_PORT(SERIAL_COM1_BASE), buf[i]);
    }
    return len;
}

void serial_write_int_dec(int val)
{
    char buf[16];
    int i = 0;
    unsigned int uval;

    if (val < 0) {
        serial_write("-", 1);
        uval = (unsigned int)(-(val + 1)) + 1;
    } else {
        uval = (unsigned int)val;
    }

    if (uval == 0) {
        serial_write("0", 1);
        return;
    }

    while (uval > 0) {
        buf[i++] = '0' + (uval % 10);
        uval /= 10;
    }

    int j;
    for (j = i - 1; j >= 0; j--) {
        serial_write(&buf[j], 1);
    }
}

void serial_write_hex(uint32 val)
{
    const char hex[] = "0123456789abcdef";
    int i;
    int started = 0;

    serial_write("0x", 2);

    for (i = 28; i >= 0; i -= 4) {
        unsigned char nibble = (val >> i) & 0xF;
        if (nibble != 0 || started || i == 0) {
            serial_write(&hex[nibble], 1);
            started = 1;
        }
    }
}