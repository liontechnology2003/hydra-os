#ifndef KERNEL_IPC_H
#define KERNEL_IPC_H

#include "types.h"
#include "syscall.h"

#define IPC_PORT_MAX    64
#define IPC_QUEUE_MAX   16
#define IPC_SVC_NAME_MAX 32

typedef struct {
    uint32 sender_pid;
    uint32 type;
    uint32 data[4];
    char   text[IPC_MSG_MAX];
    uint32 text_len;
} ipc_msg_t;

typedef struct {
    ipc_msg_t queue[IPC_QUEUE_MAX];
    uint32 head;
    uint32 tail;
    uint32 count;
    int    blocked_pid;  /* PID waiting to receive, or -1 */
} ipc_port_t;

/* Service registry entry */
typedef struct {
    char    name[IPC_SVC_NAME_MAX];
    uint32  port;
    uint32  pid;
    int     active;
} ipc_service_t;

void ipc_init(void);
int  ipc_send(uint32 dest_port, ipc_msg_t *msg);
int  ipc_receive(uint32 src_port, ipc_msg_t *msg);
int  ipc_notify(uint32 dest_pid, uint32 type);

int  ipc_service_register(const char *name, uint32 port, uint32 pid);
int  ipc_service_lookup(const char *name, ipc_service_t *out);
int  ipc_service_status(char *buf, int maxlen);

#endif /* KERNEL_IPC_H */
