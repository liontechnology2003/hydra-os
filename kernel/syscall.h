#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include "types.h"

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
#define SYS_GET_TIME    22

/* IPC message structure */
#define IPC_MSG_MAX 128

typedef struct {
    uint32 sender;
    uint32 type;
    uint32 data[4];
    char   text[IPC_MSG_MAX];
    uint32 text_len;
} __attribute__((packed)) ipc_message_t;

/* Syscall return via int 0x80 */
uint32 syscall_handler(uint32 num, uint32 a1, uint32 a2, uint32 a3, uint32 a4);

#endif /* KERNEL_SYSCALL_H */
