#ifndef INCLUDE_LINEEDIT_H
#define INCLUDE_LINEEDIT_H

#define LINEEDIT_HISTORY_SIZE 20

/** lineedit_init:
 *  Initializes the line editor.
 *
 *  @param prompt_len       Length of the current prompt string (for cursor math)
 *  @param history_buf      Buffer of LINEEDIT_HISTORY_SIZE × 256 bytes for history
 */
void lineedit_init(int prompt_len, char history_buf[][256]);

/** lineedit_update:
 *  Processes a key event and updates the display.
 *  Call this in the main loop.
 *
 *  @param buf       The working line buffer (written by the editor)
 *  @param buf_size  Size of buf
 *  @return          1 when the user pressed Enter (line is in buf), 0 otherwise
 */
int lineedit_update(char *buf, int buf_size);

/** lineedit_history_add:
 *  Adds a line to the history.
 *
 *  @param line  The command to add
 *  @param buf   The history buffer (same as passed to lineedit_init)
 */
void lineedit_history_add(const char *line, char history_buf[][256]);

#endif /* INCLUDE_LINEEDIT_H */