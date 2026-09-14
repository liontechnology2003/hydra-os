#include "pit.h"
#include "io.h"

static uint32 pit_ticks = 0;

void pit_set_tick_rate(uint32 hz)
{
    uint32 divisor = PIT_FREQUENCY / hz;
    outb(0x43, 0x36);            /* Channel 0, lo/hi, rate generator */
    outb(0x40, divisor & 0xFF);  /* Low byte */
    outb(0x40, (divisor >> 8) & 0xFF); /* High byte */
}

void pit_init(void)
{
    pit_set_tick_rate(PIT_TICK_RATE);
    pit_ticks = 0;
}

uint32 pit_get_ticks(void)
{
    return pit_ticks;
}

void pit_tick(void)
{
    pit_ticks++;
}
