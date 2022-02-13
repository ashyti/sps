#ifndef __HOST_H__
#define __HOST_H__

#include <rtdef.h>

#include "main.h"

#define SPS_HOST_ACTION_PERIOD  500 /* 5 seconds */

struct host {
    rt_thread_t action_thread;
    rt_thread_t feedback_irq_out_handler;

    /* from sps to host */
    rt_mailbox_t irq_out;
    /* from host to sps */
    rt_mailbox_t irq_in;

    enum sps_pwr_status targets[SPS_NUM_TARGETS];
    rt_mutex_t target_mutex; /* guards the targets */
};
typedef struct host *host_t;

host_t host_init(rt_mailbox_t irq_out, rt_mailbox_t irq_in);
rt_err_t host_start(host_t host);
void host_delete(host_t host);

#endif
