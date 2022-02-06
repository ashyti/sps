#include <rtthread.h>
#include <stdio.h>

#include "target.h"

struct target target = { };

rt_uint8_t do_i_freeze(rt_uint8_t probability)
{
    rt_uint8_t num = rand() % 100 ;
    return (num > probability) ? 0 : 1 ;
}

static void simulation_thread_entry(void* parameter)
{
    rt_err_t err;
    target_t target = parameter;
    rt_uint32_t gpio_status;
    rt_uint32_t ping_status;
    rt_uint32_t freeze_status;

    static enum pwr_state status = ON;

    rt_kprintf("============POWER: %d\n", status);
    err = rt_mb_recv(target->mb_gpio, &gpio_status, RT_WAITING_FOREVER);
    if(err)
    {
        rt_kprintf("Target: failed reading GPIO. Skipping\n");
    }

    switch(status)
    {
    case ON:
        if (!gpio_status)
        {
            rt_kprintf("Target: Powered OFF commandk triggered.\n");
            status = OFF;
        }
        else
        {
            freeze_status = do_i_freeze(10);
            if (freeze_status)
            {
                rt_kprintf("Target: Has frozen.\n");
                status = FROZEN;
            }
            else
            {
                //Read ping
                ping_status = 0;
                err = rt_mb_recv(target->mb_ping, &ping_status, RT_WAITING_FOREVER);
                if(err)
                {
                    rt_kprintf("Target: failed reading Ping message. Skipping\n");
                }

                if (ping_status)
                {
                    // ping back
                    rt_uint32_t msg = 1 ;
                    rt_kprintf("Target: Ping back: %d \n", msg);
                    rt_mb_send(target->mb_ping_ack, msg);
                }
            }
        }

        break;

    case OFF:
        if (gpio_status)
        {
            rt_kprintf("Target: Powered ON command triggered.\n");
            status = ON;
        }
        break;

    case FROZEN:
        if (!gpio_status)
        {
            rt_kprintf("Target: Powered OFF command triggered.\n");
            status = OFF;
        }
        else
        {
            rt_kprintf("Target: FROZEN, need to execute OFF command.\n");
        }
        break;
    }

    return;
}


target_t target_init(rt_mailbox_t mb_ping, rt_mailbox_t mb_ping_ack, rt_mailbox_t mb_gpio)
{
    //printf("Initializing target\n");

    target.simulation_thread = rt_thread_create_periodic("target_simulation",
                                                    simulation_thread_entry,
                                                    &target,
                                                    TARGET_STACK_SIZE,
                                                    TARGET_PRIORITY,
                                                    TARGET_TICK,
                                                    TARGET_PERIOD);
    target.mb_ping = mb_ping;
    target.mb_ping_ack = mb_ping_ack;
    target.mb_gpio = mb_gpio;

    //printf("Target initialized\n");

    return &target;
}


rt_err_t target_start(target_t target)
{
    //printf("Starting target`s threads\n");

    rt_thread_startup(target->simulation_thread);

    srand(time(0)); //Initialize seed for random numbers

    //printf("Target`s threads started\n");
    return RT_EOK;
}

