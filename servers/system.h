#ifndef INCLUDE_SYSTEM_H
#define INCLUDE_SYSTEM_H

void system_init(unsigned int multiboot_magic, unsigned int multiboot_info_addr);

const char *system_cpu_vendor(void);
const char *system_cpu_arch(void);
unsigned int system_ram_total_kb(void);
unsigned int system_ram_lower_kb(void);
unsigned int system_ram_upper_kb(void);
int system_ram_detected(void);

#endif /* INCLUDE_SYSTEM_H */
