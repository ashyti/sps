#include <rtthread.h>
#include <stdio.h>

#include "target.h"


extern struct target target[SPS_NUM_TARGETS] = {};

rt_uint8_t do_i_freeze(rt_uint8_t probability)
{
    rt_uint8_t num = rand() % 100 ;
    return (num >= probability) ? 0 : 1 ;
}

static void simulation_thread_entry(void* parameter)
{

    struct target *target = parameter;
    rt_err_t err;
    rt_uint32_t gpio_status;
    rt_uint32_t ping_status;
    rt_uint8_t i = target->id;

    gpio_status = 3;
    err = rt_mb_recv(target->mb_gpio, &gpio_status, 0);
    if(err)
    {
        //rt_kprintf("Target: failed reading GPIO. Skipping\n");
    }

    switch(target->status)
    {
    case ON:
        if (gpio_status == 0)
        {
            rt_kprintf("Target[%d]: Powered OFF command triggered.\n",i);
            target->status = OFF;
        }
        else
        {
            if (do_i_freeze(TARGET_FREEZE_PROB))
            {
                rt_kprintf("Target[%d]: Has frozen.\n",i);
                target->status = FROZEN;
            }
            else
            {
                //Read ping
                ping_status = 0;
                err = rt_mb_recv(target->mb_ping, &ping_status, RT_WAITING_FOREVER);
                if(err)
                {
                    rt_kprintf("Target[%d]: failed reading Ping message. Skipping\n",i);
                }

                if (ping_status == 1)
                {
                    // ping back
                    rt_uint32_t msg = 1 << i;
                    //rt_kprintf("Target[%d]: Ping back: %d \n", i, msg);
                    rt_mb_send(target->mb_ping_ack, msg);
                }
            }
        }

        break;

    case OFF:
        if (gpio_status == 1)
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

    return;
}


rt_err_t target_init(struct target *target, rt_mailbox_t mb_ping, rt_mailbox_t mb_ping_ack, rt_mailbox_t mb_gpio, rt_uint8_t i)
{
    //printf("Initializing target [%d]\n",i);
    rt_uint8_t period;
    char name[] = "targetx";
    name[sizeof(name)-2] = 48 + i;
    target->id = i;
    target->mb_ping = mb_ping;
    target->mb_ping_ack = mb_ping_ack;
    target->mb_gpio = mb_gpio;
    target->status = OFF;

    while((period = rand() % 100) < 10); //We want a tick range 10-100 (0.1 - 1 seconds)

    target->simulation_thread = rt_thread_create_periodic(name,
                                                    simulation_thread_entry,
                                                    target,
                                                    TARGET_STACK_SIZE,
                                                    TARGET_PRIORITY,
                                                    TARGET_TICK,
                                                    period);


    //printf("Target initialized [%d]\n",i);

    return RT_EOK;
}


rt_err_t target_start(struct target *target)
{
    //printf("Starting target`s threads\n");

    rt_thread_startup(target->simulation_thread);


    srand(time(0)); //Initialize seed for random numbers

    //printf("Target`s threads started\n");
    return RT_EOK;
}

