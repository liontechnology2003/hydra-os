#include "system_srv.h"
#include "../lib/syscall.h"

#define SVC_SYSTEM_PORT 13

/* Message types */
#define MSG_SYSINFO  1
#define MSG_VERSION  2
#define MSG_HOSTNAME 3

void srv_system_main(void)
{
    uipc_msg_t msg;

    sys_register_svc("system", SVC_SYSTEM_PORT);

    while (1) {
        if (sys_ipc_receive(SVC_SYSTEM_PORT, &msg) == 0) {
            switch (msg.type) {
            case MSG_SYSINFO: {
                uipc_msg_t reply;
                reply.sender = SVC_SYSTEM_PORT;
                reply.type = MSG_SYSINFO;
                reply.text_len = 0;
                sys_ipc_send(msg.sender, &reply);
                break;
            }
            case MSG_VERSION: {
                uipc_msg_t reply;
                reply.sender = SVC_SYSTEM_PORT;
                reply.type = MSG_VERSION;
                reply.text_len = 0;
                sys_ipc_send(msg.sender, &reply);
                break;
            }
            }
        }
    }
}
