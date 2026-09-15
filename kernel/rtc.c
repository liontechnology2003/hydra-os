#include "rtc.h"
#include "io.h"

#define CMOS_INDEX  0x70
#define CMOS_DATA   0x71

#define RTC_SEC     0x00
#define RTC_MIN     0x02
#define RTC_HOUR    0x04
#define RTC_DAY     0x07
#define RTC_MONTH   0x08
#define RTC_YEAR    0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

static unsigned char cmos_read(unsigned char reg)
{
    outb(CMOS_INDEX, reg);
    return inb(CMOS_DATA);
}

static unsigned char bcd_to_bin(unsigned char bcd)
{
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

void rtc_read(rtc_time_t *t)
{
    /* Wait for the RTC update-in-progress bit to clear */
    while (cmos_read(RTC_STATUS_A) & 0x80);

    t->second = bcd_to_bin(cmos_read(RTC_SEC));
    t->minute = bcd_to_bin(cmos_read(RTC_MIN));
    t->hour   = bcd_to_bin(cmos_read(RTC_HOUR));
    t->day    = bcd_to_bin(cmos_read(RTC_DAY));
    t->month  = bcd_to_bin(cmos_read(RTC_MONTH));
    t->year   = bcd_to_bin(cmos_read(RTC_YEAR)) + 2000;

    /* Read again to detect rollover (if seconds changed between reads) */
    unsigned char s2 = bcd_to_bin(cmos_read(RTC_SEC));
    if (s2 != t->second) {
        t->second = s2;
        t->minute = bcd_to_bin(cmos_read(RTC_MIN));
        t->hour   = bcd_to_bin(cmos_read(RTC_HOUR));
        t->day    = bcd_to_bin(cmos_read(RTC_DAY));
        t->month  = bcd_to_bin(cmos_read(RTC_MONTH));
        t->year   = bcd_to_bin(cmos_read(RTC_YEAR)) + 2000;
    }
}
