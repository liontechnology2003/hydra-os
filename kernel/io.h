#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

/** outb:
 *  Sends the given data to the given I/O port. Defined in io.s
 *
 *  @param port The I/O port to send the data to
 *  @param data The data to send to the I/O port
 */
void outb(unsigned short port, unsigned char data);

/** outw:
 *  Sends a 16-bit word to the given I/O port.
 *
 *  @param port The I/O port to send the data to
 *  @param data The 16-bit data to send to the I/O port
 */
void outw(unsigned short port, unsigned short data);

/** inb:
 *  Read a byte from an I/O port.
 *
 *  @param  port The address of the I/O port
 *  @return      The read byte
 */
unsigned char inb(unsigned short port);

unsigned short inw(unsigned short port);

void outl(unsigned short port, unsigned int data);

unsigned int inl(unsigned short port);

#endif /* INCLUDE_IO_H */