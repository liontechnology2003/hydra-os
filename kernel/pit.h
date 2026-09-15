#ifndef KERNEL_PIT_H
#define KERNEL_PIT_H

#include "types.h"

#define PIT_FREQUENCY 1193182
#define PIT_TICK_RATE 100  /* 100 Hz = 10ms per tick */

void pit_init(void);
void pit_set_tick_rate(uint32 hz);
uint32 pit_get_ticks(void);
void pit_tick(void);
uint32 pit_get_seconds(void);
uint32 pit_get_ms(void);

#endif /* KERNEL_PIT_H */
