#ifndef __SPS_H__
#define __SPS_H__

#include "main.h"

struct sps {
    rt_thread_t irq_in_handler;
    rt_thread_t irq_out_handler;
    /*
    rt_thread_t gpio_setting;
    rt_thread_t ping_thread;
    */

    rt_mailbox_t irq_out;
    rt_mailbox_t irq_in;
    rt_mailbox_t mb_ping;
    rt_mailbox_t gpio[SPS_NUM_TARGETS];

    rt_uint8_t targets[SPS_NUM_TARGETS];
    rt_mutex_t target_mutex; /* guards the targets */
};
typedef struct sps *sps_t;

sps_t sps_init(void);
rt_err_t sps_start(sps_t sps);
void sps_delete(sps_t sps);

#endif
