#include "fb.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "system.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"
#include "process.h"
#include "pit.h"
#include "ipc.h"

/* Server entry points (run in ring 3) */
extern void srv_display_main(void);
extern void srv_input_main(void);
extern void srv_storage_main(void);
extern void srv_system_main(void);

int kmain(unsigned int multiboot_magic, unsigned int multiboot_info_addr)
{
    /* Configure serial port */
    serial_configure_baud_rate(SERIAL_COM1_BASE, 3);
    serial_configure_line(SERIAL_COM1_BASE);
    serial_configure_fifo_buffer(SERIAL_COM1_BASE);
    serial_configure_modem(SERIAL_COM1_BASE);
    
    serial_write("Kernel starting...\n", 19);
    
    /* Set up GDT */
    gdt_install();
    tss_install(0x10, 0x90000);  /* Kernel SS=0x10, initial ESP=0x90000 */
    serial_write("GDT installed\n", 14);

    /* Detect memory from multiboot */
    system_init(multiboot_magic, multiboot_info_addr);
    serial_write("System info initialized\n", 24);

    /* Initialize physical memory manager */
    pmm_init(system_ram_total_kb());
    serial_write("PMM initialized\n", 17);

    /* Initialize paging */
    paging_init(system_ram_total_kb());
    paging_enable(paging_get_directory());
    serial_write("Paging enabled\n", 16);

    /* Initialize kernel heap */
    kheap_init();
    serial_write("Heap initialized\n", 18);
    
    /* Set up IDT */
    idt_install();
    serial_write("IDT installed\n", 14);

    /* Initialize keyboard */
    keyboard_init();
    serial_write("Keyboard initialized\n", 21);

    /* Initialize PIT timer */
    pit_init();
    serial_write("PIT initialized\n", 17);

    /* Initialize process manager */
    process_init();

    /* Initialize IPC */
    ipc_init();

    /* Spawn user-space servers */
    process_create("display", srv_display_main);
    process_create("input", srv_input_main);
    process_create("storage", srv_storage_main);
    process_create("system", srv_system_main);
    serial_write("Servers spawned\n", 16);

    /* Enable interrupts */
    __asm__ ("sti");
    serial_write("Interrupts enabled\n", 19);
    
    /* Initialize and run shell */
    shell_init();
    serial_write("Shell started\n", 14);
    
    /* Main loop */
    while (1) {
        shell_update();
    }
    
    return 0;
}
