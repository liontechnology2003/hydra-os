#ifndef KERNEL_SPLASH_H
#define KERNEL_SPLASH_H

#include "types.h"

/* Initialize the boot loading screen (must be called after gfx_init).
 * Paints the static chrome (title, bar, spinner, status) once. */
void splash_init(void);

/* Advance the boot progress: draws the bar to the given percent (0-100),
 * updates the status text, and animates the spinner for hold_ms. */
void splash_stage(int percent, const char *status, unsigned int hold_ms);

#endif /* KERNEL_SPLASH_H */