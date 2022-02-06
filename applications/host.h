#ifndef __HOST_H__
#define __HOST_H__

#include <rtdef.h>

#define SPS_HOST_ACTION_PERIOD  500

struct host {
    rt_thread_t action_thread;
    rt_thread_t feedback_irq_in_handler;

    rt_mq_t irq_out;
    rt_mq_t irq_in;

    rt_uint8_t targets[4];
};

int host_init(struct host *host);
int host_start(struct host *host);

#endif
