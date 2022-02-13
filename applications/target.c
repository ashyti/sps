#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "target.h"

static rt_uint8_t do_i_freeze(rt_uint8_t probability)
{
    rt_uint8_t num = rand() % 100 ;
    return (num >= probability) ? 0 : 1 ;
}

static void simulation_thread_entry(void* parameter)
{

    struct target *target = parameter;
    rt_ubase_t gpio_status = 3;
    rt_uint8_t i = target->id;
    rt_err_t err;

    if (target->status == ON)
    {
        rt_ubase_t ping_status;

        /* Read ping */
        err = rt_mb_recv(target->mb_ping, &ping_status, RT_WAITING_FOREVER);
        if(!err)
        {
            /* ping back */
            rt_uint32_t msg = i;
            rt_mb_send(target->mb_ping_ack, msg);
        }
    }

    err = rt_mb_recv(target->mb_gpio, &gpio_status, RT_WAITING_NO);

    switch(target->status)
    {
    case ON:
        /* the target is on and getting shut down */
        if (!err && gpio_status == 0)
        {
            rt_kprintf("Target[%d]: Powered OFF command triggered.\n", i);
            target->status = OFF;
            return;
        }

        /* the target is on and no action is taken */
        if (do_i_freeze(TARGET_FREEZE_PROB))
        {
            rt_kprintf("Target[%d]: Has frozen.\n", i);
            target->status = FROZEN;
        }

        break;

    case OFF:
        /* the target is off and getting powered on */
        if (!err && gpio_status == 1)
        {
            rt_kprintf("Target[%d]: Powered ON command triggered.\n",i);
            target->status = ON;
        }

        break;

    case FROZEN:
        if (gpio_status == 0)
        {
            rt_kprintf("Target[%d]: Powered OFF command triggered.\n",i);
            target->status = OFF;
        }
        else if (gpio_status == 1)
        {
            rt_kprintf("Target[%d]: FROZEN, need to execute OFF command.\n",i);
        }

        break;
    }
}

rt_err_t target_start(struct target *target)
{
    return rt_thread_startup(target->simulation_thread);
}

rt_err_t target_init(target_t target, rt_mailbox_t mb_ping,
                     rt_mailbox_t mb_ping_ack, rt_mailbox_t mb_gpio,
                     rt_uint8_t i)
{
    rt_uint8_t period;
    char name[] = "targetX";

    name[sizeof(name) - 2] = 48 + i;
    target->id = i;
    target->mb_ping = mb_ping;
    target->mb_ping_ack = mb_ping_ack;
    target->mb_gpio = mb_gpio;
    target->status = OFF;

    /* We want a tick range 10-100 (0.1 - 1 seconds) */
    while((period = rand() % 100) < 10)
        ;

    target->simulation_thread = rt_thread_create_periodic(name,
                                                    simulation_thread_entry,
                                                    target,
                                                    TARGET_STACK_SIZE,
                                                    TARGET_PRIORITY,
                                                    TARGET_TICK,
                                                    period);

    return target->simulation_thread ? RT_EOK : RT_ERROR;
}

void target_delete(target_t target)
{
    if (target->simulation_thread)
        rt_thread_delete(target->simulation_thread);
}
