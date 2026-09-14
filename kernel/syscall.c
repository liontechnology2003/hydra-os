#include "syscall.h"
#include "process.h"
#include "serial.h"
#include "string.h"
#include "fb.h"
#include "keyboard.h"
#include "ipc.h"

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
        /* a1 = pointer to string, a2 = length */
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
        /* a1 = fd (0=serial, 1=fb), a2 = buf, a3 = len */
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
        /* a1 = fd, a2 = buf, a3 = maxlen */
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

    default:
        serial_write("SYSCALL: unknown\n", 18);
        return (uint32)-1;
    }
}
