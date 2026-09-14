#include "gdt.h"
#include "string.h"

struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

/* 6 GDT entries: NULL, kernel code, kernel data, user code, user data, TSS */
struct gdt_entry gdt[6];
struct gdt_ptr gp;
tss_t kernel_tss;

static void gdt_set_gate(int num, unsigned int base, unsigned int limit,
                         unsigned char access, unsigned char granularity)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (granularity & 0xF0);
    gdt[num].access = access;
}

void gdt_install(void)
{
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (unsigned int)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                   /* NULL */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);    /* Kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);    /* Kernel data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);    /* User code (ring 3) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);    /* User data (ring 3) */

    load_gdt(&gp);

    /* TSS is set up later by tss_install */
}

void tss_install(uint32 kernel_ss, uint32 kernel_esp)
{
    uint32 base = (uint32)&kernel_tss;
    uint32 limit = base + sizeof(tss_t);

    memset(&kernel_tss, 0, sizeof(tss_t));

    kernel_tss.ss0 = kernel_ss;
    kernel_tss.esp0 = kernel_esp;
    kernel_tss.cs = 0x08 | 3;   /* Kernel code, ring 3 (for iret back) */
    kernel_tss.ss = 0x10 | 3;
    kernel_tss.ds = 0x10 | 3;
    kernel_tss.es = 0x10 | 3;
    kernel_tss.fs = 0x10 | 3;
    kernel_tss.gs = 0x10 | 3;
    kernel_tss.iomap_base = sizeof(tss_t);

    /* GDT entry 5 = TSS */
    gdt_set_gate(5, base, limit, 0xE9, 0x00);

    /* Load TSS */
    __asm__ volatile ("ltr %%ax" : : "a"((uint16)0x28));
}

void tss_set_kernel_stack(uint32 esp)
{
    kernel_tss.esp0 = esp;
}
