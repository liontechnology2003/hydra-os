#ifndef INCLUDE_SHELL_H
#define INCLUDE_SHELL_H

/** shell_init:
 *  Initializes the shell and draws the welcome banner.
 */
void shell_init(void);

/** shell_update:
 *  Updates the shell (call this in main loop).
 */
void shell_update(void);

/** shell_puts:
 *  Writes a string to the current stream (stdout: fb, file, or pipe).
 */
void shell_puts(const char *str);

/** shell_putc:
 *  Writes a character to the current stream.
 */
void shell_putc(char c);

/** shell_print_int:
 *  Prints an integer to the current stream.
 */
void shell_print_int(int val);

/** shell_print_unsigned:
 *  Prints an unsigned integer to the current stream.
 */
void shell_print_unsigned(unsigned int val);

#endif /* INCLUDE_SHELL_H */