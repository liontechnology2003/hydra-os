#include "system.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEMORY      0x00000001

typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms[4];
    unsigned int mmap_length;
    unsigned int mmap_addr;
} multiboot_info;

static char cpu_vendor[13] = "unknown";
static unsigned int ram_total_kb = 32768;
static unsigned int ram_lower_kb = 640;
static unsigned int ram_upper_kb = 31744;
static int ram_detected = 0;

static void system_detect_cpu(void)
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    eax = 0;
    __asm__ volatile ("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(eax));

    cpu_vendor[0] = (char)(ebx & 0xff);
    cpu_vendor[1] = (char)((ebx >> 8) & 0xff);
    cpu_vendor[2] = (char)((ebx >> 16) & 0xff);
    cpu_vendor[3] = (char)((ebx >> 24) & 0xff);
    cpu_vendor[4] = (char)(edx & 0xff);
    cpu_vendor[5] = (char)((edx >> 8) & 0xff);
    cpu_vendor[6] = (char)((edx >> 16) & 0xff);
    cpu_vendor[7] = (char)((edx >> 24) & 0xff);
    cpu_vendor[8] = (char)(ecx & 0xff);
    cpu_vendor[9] = (char)((ecx >> 8) & 0xff);
    cpu_vendor[10] = (char)((ecx >> 16) & 0xff);
    cpu_vendor[11] = (char)((ecx >> 24) & 0xff);
    cpu_vendor[12] = '\0';
}

void system_init(unsigned int multiboot_magic, unsigned int multiboot_info_addr)
{
    multiboot_info *mbi;

    system_detect_cpu();

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC || multiboot_info_addr == 0) {
        return;
    }

    mbi = (multiboot_info *)multiboot_info_addr;
    if ((mbi->flags & MULTIBOOT_INFO_MEMORY) == 0) {
        return;
    }

    ram_lower_kb = mbi->mem_lower;
    ram_upper_kb = mbi->mem_upper;
    ram_total_kb = mbi->mem_upper + 1024;
    ram_detected = 1;
}

const char *system_cpu_vendor(void)
{
    return cpu_vendor;
}

const char *system_cpu_arch(void)
{
    return "i386";
}

unsigned int system_ram_total_kb(void)
{
    return ram_total_kb;
}

unsigned int system_ram_lower_kb(void)
{
    return ram_lower_kb;
}

unsigned int system_ram_upper_kb(void)
{
    return ram_upper_kb;
}

int system_ram_detected(void)
{
    return ram_detected;
}
