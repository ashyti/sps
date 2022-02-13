#ifndef __TARGET_H__
#define __TARGET_H__

#include "main.h"

enum sps_pwr_status
{
    OFF = 100,
    ON,
    FROZEN,
};

struct target
{
    rt_thread_t simulation_thread;      /**< Simulation Thread */

    rt_mailbox_t mb_gpio;               /**< GPIO mailbox */
    rt_mailbox_t mb_ping;               /**< Ping mailbox -From SPS*/
    rt_mailbox_t mb_ping_ack;           /**< Ping ACK mailbox -To SPS*/
    rt_uint8_t id;                      /**< Address */
    enum sps_pwr_status status;         /**< Power Status */
};
typedef struct target *target_t;

rt_err_t target_start(struct target *target);
rt_err_t target_init(struct target *target, rt_mailbox_t mb_ping,
                       rt_mailbox_t mb_ping_ack, rt_mailbox_t mb_gpio,
                       rt_uint8_t i);
void target_delete(target_t target);

#endif
