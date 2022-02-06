#include <rtthread.h>
#include <stdio.h>

#include "target.h"

struct target target = { };

static void simulation_thread_entry(void* parameter)
{
    rt_err_t err;
    target_t target1 = parameter;
    rt_uint32_t gpio_status;
    rt_uint32_t ping_status;
/*
    err = rt_mb_recv(target1->mb_gpio, &gpio_status, RT_WAITING_FOREVER);
    if(err)
    {
        //handle error
    }

    else
    {
        //rt_kprintf("This is the message:%d\n", gpio_status);
        if (gpio_status == 0)
        {
            //stop and do nothing
            rt_kprintf("Stopping\n");
        }

        else{
            //Read ping
            err = rt_mb_recv(target1->mb_ping, &ping_status, RT_WAITING_FOREVER);
            if(err)
            {
                //handle error
            }

            else
            {
                if (ping_status)
                {
                    // ping back
                }

            }
        }
    }
    */
    err = rt_mb_recv(target1->mb_ping, &ping_status, RT_WAITING_FOREVER);
    rt_kprintf("Receiving Ping: %d\n", ping_status);

}


target_t target_init(rt_mailbox_t mb_ping, rt_mailbox_t mb_gpio)
{
    printf("Initializing target\n");

    target.simulation_thread = rt_thread_create_periodic("target_simulation",
                                                    simulation_thread_entry,
                                                    &target,
                                                    TARGET_STACK_SIZE,
                                                    TARGET_PRIORITY,
                                                    TARGET_TICK,
                                                    TARGET_PERIOD);
    target.mb_ping = mb_ping;
    target.mb_gpio = mb_gpio;

    printf("Target initialized\n");

    return &target;
}


rt_err_t target_start(target_t target)
{
    printf("Starting target`s threads\n");

    rt_thread_startup(target->simulation_thread);

    printf("Target`s threads started\n");
    return RT_EOK;
}

