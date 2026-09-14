#include "display.h"
#include "../lib/syscall.h"

#define SVC_DISPLAY_PORT 10

/* Message types */
#define MSG_FB_WRITE    1
#define MSG_FB_CLEAR    2
#define MSG_FB_SETCOLOR 3

void srv_display_main(void)
{
    uipc_msg_t msg;

    sys_register_svc("display", SVC_DISPLAY_PORT);

    while (1) {
        if (sys_ipc_receive(SVC_DISPLAY_PORT, &msg) == 0) {
            switch (msg.type) {
            case MSG_FB_WRITE:
                sys_fb_write(msg.text, msg.text_len);
                break;
            case MSG_FB_CLEAR:
                /* TODO: fb_clear via syscall */
                break;
            }
        }
    }
}
