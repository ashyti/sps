#ifndef __TARGET_H__
#define __TARGET_H__

#include "main.h"



#define TARGET_PERIOD           100
#define TARGET_STACK_SIZE       512
#define TARGET_PRIORITY         1
#define TARGET_TICK             1

enum pwr_status{ON, OFF, FROZEN};

struct target
{
    rt_thread_t simulation_thread;      /**< Simulation Thread */

    rt_mailbox_t mb_gpio;               /**< GPIO mailbox */
    rt_mailbox_t mb_ping;               /**< Ping mailbox -From SPS*/
    rt_mailbox_t mb_ping_ack;            /**< Ping ACK mailbox -To SPS*/
    rt_uint8_t id;
    enum pwr_status status;
};
typedef struct target *target_t;

extern struct target target[SPS_NUM_TARGETS];



rt_uint8_t do_i_freeze(rt_uint8_t probability);
static void simulation_thread_entry(void *param);
void target_init(struct target *target, rt_mailbox_t mb_ping, rt_mailbox_t mb_ping_ack, rt_mailbox_t mb_gpio, rt_uint8_t i);
rt_err_t target_start(struct target *target);

#endif
