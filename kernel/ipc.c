#include "ipc.h"
#include "process.h"
#include "kheap.h"
#include "string.h"
#include "serial.h"

static ipc_port_t ports[IPC_PORT_MAX];
static ipc_service_t services[IPC_PORT_MAX];
static int service_count = 0;

void ipc_init(void)
{
    int i;
    for (i = 0; i < IPC_PORT_MAX; i++) {
        ports[i].head = 0;
        ports[i].tail = 0;
        ports[i].count = 0;
        ports[i].blocked_pid = -1;
    }
    service_count = 0;
    serial_write("IPC: initialized\n", 17);
}

int ipc_send(uint32 dest_port, ipc_msg_t *msg)
{
    ipc_port_t *port;

    if (dest_port >= IPC_PORT_MAX || !msg) {
        return -1;
    }

    port = &ports[dest_port];

    /* If someone is blocked waiting, wake them up directly */
    if (port->blocked_pid != -1) {
        process_t *target = process_current();
        (void)target;
        /* Copy message to the receiver's buffer - the blocked process
         * will find it when it resumes */
        port->blocked_pid = -1;
    }

    /* Enqueue message */
    if (port->count >= IPC_QUEUE_MAX) {
        return -1;  /* Queue full */
    }

    port->queue[port->tail] = *msg;
    port->tail = (port->tail + 1) % IPC_QUEUE_MAX;
    port->count++;

    return 0;
}

int ipc_receive(uint32 src_port, ipc_msg_t *msg)
{
    ipc_port_t *port;

    if (src_port >= IPC_PORT_MAX || !msg) {
        return -1;
    }

    port = &ports[src_port];

    /* If message available, return it */
    if (port->count > 0) {
        *msg = port->queue[port->head];
        port->head = (port->head + 1) % IPC_QUEUE_MAX;
        port->count--;
        return 0;
    }

    /* No message - block current process */
    if (current_proc) {
        port->blocked_pid = current_proc->pid;
        current_proc->state = PROC_BLOCKED;
        process_schedule();
        /* When we return, a message should be available */
        if (port->count > 0) {
            *msg = port->queue[port->head];
            port->head = (port->head + 1) % IPC_QUEUE_MAX;
            port->count--;
            return 0;
        }
    }

    return -1;
}

int ipc_notify(uint32 dest_pid, uint32 type)
{
    ipc_msg_t msg;
    int i;

    memset(&msg, 0, sizeof(msg));
    msg.sender_pid = 0;
    msg.type = type;

    /* Find a port owned by this PID */
    for (i = 0; i < IPC_PORT_MAX; i++) {
        if (ports[i].blocked_pid == (int)dest_pid) {
            return ipc_send(i, &msg);
        }
    }

    return -1;
}

int ipc_service_register(const char *name, uint32 port, uint32 pid)
{
    int i;

    if (service_count >= IPC_PORT_MAX) {
        return -1;
    }

    /* Check for duplicate */
    for (i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            services[i].port = port;
            services[i].pid = pid;
            services[i].active = 1;
            return 0;
        }
    }

    strncpy(services[service_count].name, name, IPC_SVC_NAME_MAX - 1);
    services[service_count].name[IPC_SVC_NAME_MAX - 1] = '\0';
    services[service_count].port = port;
    services[service_count].pid = pid;
    services[service_count].active = 1;
    service_count++;

    serial_write("IPC: service registered: ", 25);
    serial_write((char *)name, strlen(name));
    serial_write("\n", 1);

    return 0;
}

int ipc_service_lookup(const char *name, ipc_service_t *out)
{
    int i;

    for (i = 0; i < service_count; i++) {
        if (services[i].active && strcmp(services[i].name, name) == 0) {
            if (out) *out = services[i];
            return 0;
        }
    }
    return -1;
}

int ipc_service_status(char *buf, int maxlen)
{
    int i;
    int pos = 0;

    for (i = 0; i < service_count && pos < maxlen - 1; i++) {
        int j;
        for (j = 0; services[i].name[j] && pos < maxlen - 1; j++) {
            buf[pos++] = services[i].name[j];
        }
        buf[pos++] = ' ';
        {
            char pbuf[8];
            utoa(pbuf, services[i].port);
            for (j = 0; pbuf[j] && pos < maxlen - 1; j++) {
                buf[pos++] = pbuf[j];
            }
        }
        buf[pos++] = ' ';
        {
            char pbuf[8];
            utoa(pbuf, services[i].pid);
            for (j = 0; pbuf[j] && pos < maxlen - 1; j++) {
                buf[pos++] = pbuf[j];
            }
        }
        buf[pos++] = '\n';
    }

    if (pos < maxlen) buf[pos] = '\0';
    return pos;
}
