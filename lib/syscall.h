#ifndef ULIB_SYSCALL_H
#define ULIB_SYSCALL_H

#include "types.h"

/* IPC message (must match kernel ipc.h) */
#define IPC_MSG_MAX 128

typedef struct {
    uint32 sender;
    uint32 type;
    uint32 data[4];
    char   text[IPC_MSG_MAX];
    uint32 text_len;
} __attribute__((packed)) uipc_msg_t;

/* Syscall numbers */
#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2
#define SYS_GETPID      3
#define SYS_FORK        4
#define SYS_EXEC        5
#define SYS_IPC_SEND    6
#define SYS_IPC_RECEIVE 7
#define SYS_IPC_NOTIFY  8
#define SYS_REGISTER_SVC 9
#define SYS_GET_SVC     10
#define SYS_FB_WRITE    11
#define SYS_KBD_GET     12
#define SYS_SVC_STATUS  13
#define SYS_REBOOT      14
#define SYS_SHUTDOWN    15
#define SYS_UPTIME      16
#define SYS_FB_FLUSH    17
#define SYS_FB_GET_INFO 18
#define SYS_MOUSE_GET   19
#define SYS_KBD_EVENT   20
#define SYS_FB_MAP      21

/* Inline syscall wrapper */
static inline uint32 syscall(uint32 num, uint32 a1, uint32 a2, uint32 a3, uint32 a4)
{
    uint32 ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a1), "c"(a2), "d"(a3), "S"(a4)
        : "memory"
    );
    return ret;
}

static inline void sys_exit(void)                          { syscall(SYS_EXIT, 0, 0, 0, 0); }
static inline int  sys_getpid(void)                        { return (int)syscall(SYS_GETPID, 0, 0, 0, 0); }
static inline int  sys_fork(void)                          { return (int)syscall(SYS_FORK, 0, 0, 0, 0); }
static inline int  sys_write(int fd, const char *buf, int len) { return (int)syscall(SYS_WRITE, fd, (uint32)buf, len, 0); }
static inline int  sys_read(int fd, char *buf, int maxlen)    { return (int)syscall(SYS_READ, fd, (uint32)buf, maxlen, 0); }
static inline int  sys_fb_write(const char *buf, int len)     { return (int)syscall(SYS_FB_WRITE, (uint32)buf, len, 0, 0); }
static inline int  sys_kbd_get(void)                       { return (int)syscall(SYS_KBD_GET, 0, 0, 0, 0); }
static inline int  sys_ipc_send(uint32 dest, uipc_msg_t *msg) { return (int)syscall(SYS_IPC_SEND, dest, (uint32)msg, 0, 0); }
static inline int  sys_ipc_receive(uint32 src, uipc_msg_t *msg) { return (int)syscall(SYS_IPC_RECEIVE, src, (uint32)msg, 0, 0); }
static inline int  sys_register_svc(const char *name, uint32 port) { return (int)syscall(SYS_REGISTER_SVC, (uint32)name, port, sys_getpid(), 0); }
static inline void sys_reboot(void)    { syscall(SYS_REBOOT, 0, 0, 0, 0); }
static inline void sys_shutdown(void)  { syscall(SYS_SHUTDOWN, 0, 0, 0, 0); }
static inline uint32 sys_uptime(void)  { return syscall(SYS_UPTIME, 0, 0, 0, 0); }
static inline uint32 *sys_fb_get_buffer(void) { return (uint32 *)syscall(SYS_FB_MAP, 0, 0, 0, 0); }
static inline void sys_fb_release_buffer(uint32 ptr) { (void)ptr; }
static inline void sys_fb_flush(void)  { syscall(SYS_FB_FLUSH, 0, 0, 0, 0); }
static inline void sys_mouse_get(int32 *x, int32 *y, int32 *btn) {
    uint32 ret = syscall(SYS_MOUSE_GET, 0, 0, 0, 0);
    if (x) *x = (int32)(ret & 0xFFFF);
    if (y) *y = (int32)((ret >> 16) & 0xFFFF);
    if (btn) *btn = (int32)(syscall(SYS_MOUSE_GET, 1, 0, 0, 0));
}
static inline int32 sys_mouse_get_scroll(void) {
    return (int32)syscall(SYS_MOUSE_GET, 2, 0, 0, 0);
}
/* Keyboard event types (must match kernel keyboard.h) */
#define KBD_EV_NONE             0
#define KBD_EV_CHAR             1
#define KBD_EV_LEFT             2
#define KBD_EV_RIGHT            3
#define KBD_EV_UP               4
#define KBD_EV_DOWN             5
#define KBD_EV_HOME             6
#define KBD_EV_END              7
#define KBD_EV_DEL              8
#define KBD_EV_TAB              9
#define KBD_EV_BACKSPACE        10
#define KBD_EV_ENTER            11

/* Returns event type, or KBD_EV_NONE. On KBD_EV_CHAR, *key holds the char. */
static inline int sys_kbd_get_event(int32 *key) {
    uint32 ret = syscall(SYS_KBD_EVENT, 0, 0, 0, 0);
    int type = (int)((ret >> 8) & 0xFF);
    if (type != KBD_EV_NONE && key) *key = (int32)(ret & 0xFF);
    return type;
}

#endif /* ULIB_SYSCALL_H */
