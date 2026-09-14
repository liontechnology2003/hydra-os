#include "storage.h"
#include "../lib/syscall.h"

#define SVC_STORAGE_PORT 12

/* Message types */
#define MSG_VFS_READ   1
#define MSG_VFS_WRITE  2
#define MSG_VFS_LS     3
#define MSG_VFS_MKDIR  4
#define MSG_VFS_RM     5
#define MSG_VFS_RESOLVE 6

void srv_storage_main(void)
{
    uipc_msg_t msg;

    sys_register_svc("storage", SVC_STORAGE_PORT);

    while (1) {
        if (sys_ipc_receive(SVC_STORAGE_PORT, &msg) == 0) {
            switch (msg.type) {
            case MSG_VFS_READ: {
                /* TODO: forward to VFS */
                uipc_msg_t reply;
                reply.sender = SVC_STORAGE_PORT;
                reply.type = MSG_VFS_READ;
                reply.data[0] = 0;
                sys_ipc_send(msg.sender, &reply);
                break;
            }
            case MSG_VFS_WRITE: {
                /* TODO: forward to VFS */
                uipc_msg_t reply;
                reply.sender = SVC_STORAGE_PORT;
                reply.type = MSG_VFS_WRITE;
                reply.data[0] = 0;
                sys_ipc_send(msg.sender, &reply);
                break;
            }
            }
        }
    }
}
