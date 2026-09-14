#include "input.h"
#include "../lib/syscall.h"

#define SVC_INPUT_PORT 11

/* Message types */
#define MSG_KEY_GET  1
#define MSG_KEY_WAIT 2

void srv_input_main(void)
{
    uipc_msg_t msg;

    sys_register_svc("input", SVC_INPUT_PORT);

    while (1) {
        if (sys_ipc_receive(SVC_INPUT_PORT, &msg) == 0) {
            switch (msg.type) {
            case MSG_KEY_GET: {
                int ch = sys_kbd_get();
                uipc_msg_t reply;
                reply.sender = SVC_INPUT_PORT;
                reply.type = MSG_KEY_GET;
                reply.data[0] = ch;
                sys_ipc_send(msg.sender, &reply);
                break;
            }
            }
        }
    }
}
