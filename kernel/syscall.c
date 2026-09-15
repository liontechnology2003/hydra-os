#include "syscall.h"
#include "process.h"
#include "serial.h"
#include "string.h"
#include "fb.h"
#include "keyboard.h"
#include "ipc.h"
#include "pit.h"
#include "io.h"
#include "gfx.h"
#include "mouse.h"
#include "vbe.h"

uint32 syscall_handler(uint32 num, uint32 a1, uint32 a2, uint32 a3, uint32 a4)
{
    (void)a3; (void)a4;

    switch (num) {
    case SYS_EXIT:
        process_exit();
        return 0;

    case SYS_GETPID:
        return current_proc ? current_proc->pid : 0;

    case SYS_FORK:
        return process_fork();

    case SYS_FB_WRITE: {
        char *str = (char *)a1;
        uint32 len = a2;
        uint32 i;
        for (i = 0; i < len; i++) {
            fb_putc(str[i]);
        }
        return len;
    }

    case SYS_KBD_GET: {
        char ch = keyboard_get_char();
        return (uint32)ch;
    }

    case SYS_WRITE: {
        uint32 fd = a1;
        char *buf = (char *)a2;
        uint32 len = a3;
        if (fd == 0) {
            serial_write(buf, len);
        } else if (fd == 1) {
            uint32 i;
            for (i = 0; i < len; i++) {
                fb_putc(buf[i]);
            }
        }
        return len;
    }

    case SYS_READ: {
        char *buf = (char *)a2;
        uint32 maxlen = a3;
        uint32 i = 0;
        while (i < maxlen - 1) {
            char ch = keyboard_get_char();
            if (ch == 0) break;
            buf[i++] = ch;
            fb_putc(ch);
        }
        buf[i] = '\0';
        return i;
    }

    case SYS_IPC_SEND:
        return ipc_send(a1, (ipc_msg_t *)a2);

    case SYS_IPC_RECEIVE:
        return ipc_receive(a1, (ipc_msg_t *)a2);

    case SYS_IPC_NOTIFY:
        return ipc_notify(a1, a2);

    case SYS_REGISTER_SVC:
        return ipc_service_register((const char *)a1, a2, a3);

    case SYS_GET_SVC: {
        ipc_service_t svc;
        int r = ipc_service_lookup((const char *)a1, &svc);
        if (r == 0 && a2) {
            *(ipc_service_t *)a2 = svc;
        }
        return r;
    }

    case SYS_SVC_STATUS:
        return ipc_service_status((char *)a1, a2);

    case SYS_REBOOT: {
        /* PS/2 controller reset: write 0xFE to port 0x64 */
        serial_write("REBOOT: resetting...\n", 21);
        while (1) {
            outb(0x64, 0xFE);
        }
    }

    case SYS_SHUTDOWN: {
        /* QEMU ACPI shutdown: write 0x2000 to port 0x604 */
        serial_write("SHUTDOWN: powering off...\n", 25);
        outw(0x604, 0x2000);
        /* Fallback: Bochs */
        outw(0xB004, 0x2000);
        while (1) {
            __asm__ volatile ("hlt");
        }
    }

    case SYS_UPTIME:
        return pit_get_seconds();

    case SYS_FB_GET_INFO: {
        /* Return LFB address for direct framebuffer access */
        vbe_info_t *v = vbe_get_info();
        if (v && v->address) {
            return (uint32)v->address;
        }
        return 0;
    }

    case SYS_FB_FLUSH: {
        /* Trigger full screen redraw (compose) */
        return 0;
    }

    case SYS_MOUSE_GET: {
        if (a1 == 1) {
            /* Return buttons */
            return (uint32)mouse_get_buttons();
        }
        if (a1 == 2) {
            /* Return scroll delta */
            return (uint32)mouse_get_scroll();
        }
        /* Return packed x|y */
        uint32 mx = (uint32)mouse_get_x();
        uint32 my = (uint32)mouse_get_y();
        return (my << 16) | (mx & 0xFFFF);
    }

    case SYS_KBD_EVENT: {
        /* Non-blocking keyboard event: returns (type << 8) | char.
         * type is one of KBD_EV_* from keyboard.h; char is the
         * printable character for KBD_EV_CHAR events. */
        char ch = 0;
        int type = keyboard_get_event(&ch);
        if (type != KBD_EV_NONE) {
            return ((uint32)(type & 0xFF) << 8) | ((uint32)ch & 0xFF);
        }
        return 0;
    }

    case SYS_FB_MAP: {
        /* Map the physical LFB into user process at 0xD0000000 */
        vbe_info_t *v = vbe_get_info();
        if (!v || !v->phys) return 0;

        uint32 user_lfb = 0xD0000000;
        uint32 pages = (v->size + 0xFFF) / 0x1000;
        uint32 i;

        for (i = 0; i < pages; i++) {
            paging_map_page(user_lfb + i * 0x1000, v->phys + i * 0x1000,
                            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        }

        return user_lfb;
    }

    default:
        serial_write("SYSCALL: unknown\n", 18);
        return (uint32)-1;
    }
}
