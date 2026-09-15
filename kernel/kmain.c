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
#include "cpp_runtime.h"
#include "vbe.h"
#include "gfx.h"
#include "splash.h"
#include "mouse.h"

/* Server entry points (run in ring 3) */
extern void srv_display_main(void);
extern void srv_input_main(void);
extern void srv_storage_main(void);
extern void srv_system_main(void);
extern void srv_wm_main(void);

/* Boot mode: 1=Desktop, 2=Shell */
static int boot_mode = 1;

static void draw_boot_menu(void)
{
    fb_clear();
    fb_set_color(FB_WHITE, FB_BLACK);
    fb_puts("========================================\n");
    fb_set_color(FB_RED, FB_BLACK);
    fb_puts("         RedLion OS 2.0.0\n");
    fb_set_color(FB_WHITE, FB_BLACK);
    fb_puts("========================================\n");
    fb_puts("\n");
    fb_puts("  Welcome to RedLion OS!\n");
    fb_puts("\n");
    fb_puts("  Select boot mode:\n");
    fb_puts("\n");
    fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
    fb_puts("  [1] Desktop  (graphical window manager)\n");
    fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
    fb_puts("  [2] Shell    (text-mode console)\n");
    fb_set_color(FB_WHITE, FB_BLACK);
    fb_puts("\n");
    fb_set_color(FB_DARK_GREY, FB_BLACK);
    fb_puts("  Default: Desktop in 3 seconds...\n");
    fb_set_color(FB_WHITE, FB_BLACK);
    fb_puts("\n");
    fb_puts("  Press 1 or 2 to choose: ");
}

static int wait_for_choice(void)
{
    uint32 ticks_start;
    uint32 timeout_ms = 3000;

    ticks_start = pit_get_ticks();

    while (1) {
        char ch = keyboard_get_char();
        if (ch == '1') return 1;
        if (ch == '2') return 2;

        uint32 elapsed = (pit_get_ticks() - ticks_start) * 10;
        if (elapsed >= timeout_ms) {
            return 1; /* default: Desktop */
        }
    }
}

int kmain(unsigned int multiboot_magic, unsigned int multiboot_info_addr)
{
    int desktop_mode;

    /* Configure serial port */
    serial_configure_baud_rate(SERIAL_COM1_BASE, 3);
    serial_configure_line(SERIAL_COM1_BASE);
    serial_configure_fifo_buffer(SERIAL_COM1_BASE);
    serial_configure_modem(SERIAL_COM1_BASE);
    
    serial_write("Kernel starting...\n", 19);
    
    /* Set up GDT */
    gdt_install();
    tss_install(0x10, 0x90000);
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

    /* Set up IDT early so faults are caught */
    idt_install();
    serial_write("IDT installed\n", 14);

    /* Initialize PIT timer */
    pit_init();
    serial_write("PIT initialized\n", 17);

    /* Initialize keyboard (needed for boot menu) */
    keyboard_init();
    serial_write("Keyboard initialized\n", 21);

    /* Enable interrupts (needed for keyboard/PIT) */
    __asm__ ("sti");
    serial_write("Interrupts enabled\n", 19);

    /* Run C++ global constructors */
    __libc_init_array();
    serial_write("Global constructors called\n", 28);

    /* ---- Boot Menu ---- */
    draw_boot_menu();
    boot_mode = wait_for_choice();
    desktop_mode = (boot_mode == 1);

    if (desktop_mode) {
        serial_write("BOOT: Desktop mode selected\n", 28);
    fb_set_color(FB_LIGHT_GREEN, FB_BLACK);
        fb_puts("\n  Starting Desktop...\n");
    } else {
        serial_write("BOOT: Shell mode selected\n", 26);
        fb_set_color(FB_LIGHT_CYAN, FB_BLACK);
        fb_puts("\n  Starting Shell...\n");
    }

    if (desktop_mode) {
        /* Initialize VBE framebuffer */
        if (vbe_init(multiboot_info_addr) == 0) {
            gfx_init();
            splash_init();
            serial_write("GFX: graphical framebuffer active\n", 35);
        } else {
            serial_write("GFX: no framebuffer, falling back to text\n", 43);
            desktop_mode = 0;
        }
    }

    /* Initialize PS/2 mouse (only useful in desktop) */
    if (desktop_mode) {
        splash_stage(15, "Initializing mouse...", 250);
        mouse_init();
        serial_write("Mouse initialized\n", 18);
        splash_stage(35, "Mouse ready", 200);
    }

    /* Initialize process manager */
    splash_stage(40, "Starting processes...", 150);
    process_init();

    /* Initialize IPC */
    ipc_init();

    /* Spawn user-space servers */
    splash_stage(55, "Loading services...", 150);
    process_create("display", srv_display_main);
    process_create("input", srv_input_main);
    process_create("storage", srv_storage_main);
    process_create("system", srv_system_main);
    if (desktop_mode) {
        process_create("wm", (void *)srv_wm_main);
    }
    serial_write("Servers spawned\n", 16);

    /* Initialize and run shell */
    splash_stage(75, "Setting up filesystem...", 200);
    shell_init();
    serial_write("Shell started\n", 14);
    
    /* Main loop */
    if (desktop_mode) {
        splash_stage(100, "Launching window manager...", 250);
        serial_write("WM: entering window manager\n", 28);
        srv_wm_main();
    }
    while (1) {
        shell_update();
    }
    
    return 0;
}
