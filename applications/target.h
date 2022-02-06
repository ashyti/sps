#ifndef __TARGET_H__
#define __TARGET_H__

#define TARGET_PERIOD           500
#define TARGET_STACK_SIZE       512
#define TARGET_PRIORITY         1
#define TARGET_TICK             1

struct target
{
    rt_thread_t simulation_thread;      /**< Simulation Thread */

    rt_mailbox_t mb_gpio;               /**< GPIO mailbox */
    //rt_mailbox_t mb_ping;               /**< Ping mailbox */
    char mb_gpio_pool[128];
    rt_uint32_t gpio;                                    /**< GPIO message */
    rt_uint32_t ping;                                    /**< Ping message */
};

static void simulation_thread_entry(void *param);
rt_err_t target_init(struct target *target);
rt_err_t target_start(struct target *target);

#endif
