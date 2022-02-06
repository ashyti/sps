#include <rtthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "host.h"

struct host host = { };

static void action_thread(void *param)
{
    rt_uint8_t changed = false;
    struct host *host = param;
    rt_ubase_t targets = 0;
    rt_err_t err;
    int i;

    err = rt_sem_take(host->target_sem, RT_WAITING_FOREVER);
    if (err)
        /* handle error */ ;
    for (i = 0; i < sizeof(targets); i++)
    {
        int choice = rand() % 10;

        /* we want 90% of chance that the status is not changed */
        if (choice >= 9)
        {
            changed = true;
            host->targets[i] = !host->targets[i];
        }

        if (host->targets[i])
            targets |= (1 << i);
        else
            targets &= ~(1 << i); 
    }
    rt_sem_release(host->target_sem);

    if (changed)
    {
        rt_kprintf("%d - %s, %lx (CHANGED)\n", __LINE__, __func__, targets);
        rt_mb_send(host->irq_in, targets);
    }
    else
    {
        //rt_kprintf("%d - %s, %lx\n", __LINE__, __func__, targets);
    }
}

static void feedback_irq_out(void *param)
{
    struct host *host = param;
    rt_err_t err;

    while (1)
    {
        rt_ubase_t targets;
        int i;

        err = rt_mb_recv(host->irq_out, &targets, RT_WAITING_FOREVER);
        if (err)
            /* handle error */ ;

        err = rt_sem_take(host->target_sem, RT_WAITING_FOREVER);
        if (err)
            /* handle error */ ;

        for (i = 0; i < SPS_NUM_TARGETS; i++) {
            rt_uint8_t current_target = (targets >> i) & 0x1;

            host->targets[i] = !!current_target;
        }
        rt_sem_release(host->target_sem);
    }
}

rt_err_t host_start(host_t host)
{
    rt_thread_startup(host->action_thread);
    rt_thread_startup(host->feedback_irq_out_handler);

    return RT_EOK;
}

host_t host_init(rt_mailbox_t irq_out, rt_mailbox_t irq_in)
{
    printf("I'm the host initializer\n");

    host.action_thread = rt_thread_create_periodic("host_action",
                                                   action_thread, &host,
                                                   SPS_THREAD_STACK_SIZE,
                                                   1, 1,
                                                   SPS_HOST_ACTION_PERIOD);

    host.feedback_irq_out_handler = rt_thread_create("host_irq_out",
                                                     feedback_irq_out, &host,
                                                     SPS_THREAD_STACK_SIZE,
                                                     1, 1);

    host.irq_out = irq_out;
    host.irq_in = irq_in;

    host.target_sem = rt_sem_create("host_target_sem", 0, RT_IPC_FLAG_FIFO);
    if (!host.target_sem)
        return RT_NULL;

    printf("Host initialized\n");

    return &host;
}
