#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H

#include "types.h"

typedef struct {
    uint8  second;
    uint8  minute;
    uint8  hour;
    uint8  day;
    uint8  month;
    uint16 year;
} rtc_time_t;

/* Read the current wall-clock time from the CMOS RTC.
 * All values are in BCD-converted decimal (not raw BCD). */
void rtc_read(rtc_time_t *t);

#endif /* KERNEL_RTC_H */
