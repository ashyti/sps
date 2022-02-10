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

    err = rt_mutex_take(host->target_mutex, RT_WAITING_FOREVER);
    if (err)
    {
        rt_kprintf("Host: failed to synchronise. Skipping\n");
        return;
    }

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
    }
    rt_mutex_release(host->target_mutex);

    if (changed)
    {
        err = rt_mb_send(host->irq_in, targets);
        if (err)
            rt_kprintf("Host: failed to send command. Skipping\n");
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
        {
            rt_kprintf("Host: failed to receive message from SPS. Skipping\n");
            continue;
        }

        err = rt_mutex_take(host->target_mutex, RT_WAITING_FOREVER);
        if (err)
        {
            rt_kprintf("Host: failed to synchronise. Skipping\n");
            continue;
        }

        for (i = 0; i < SPS_NUM_TARGETS; i++)
        {
            rt_uint8_t current_target = (targets >> i) & 0x1;

            host->targets[i] = !!current_target;

            if (current_target == 0)
            {
                printf("SPS:Target[%d] is DOWN\n", i);
            }
        }
        rt_mutex_release(host->target_mutex);
    }
}

rt_err_t host_start(host_t host)
{
    rt_err_t err;

    err = rt_thread_startup(host->action_thread);
    if (err)
        return err;

    return rt_thread_startup(host->feedback_irq_out_handler);
}

host_t host_init(rt_mailbox_t irq_out, rt_mailbox_t irq_in)
{
    host.action_thread = rt_thread_create_periodic("host_action",
                                                   action_thread, &host,
                                                   SPS_THREAD_STACK_SIZE,
                                                   1, 1,
                                                   SPS_HOST_ACTION_PERIOD);
    if (!host.action_thread)
        return RT_NULL;

    host.feedback_irq_out_handler = rt_thread_create("host_irq_out",
                                                     feedback_irq_out, &host,
                                                     SPS_THREAD_STACK_SIZE,
                                                     1, 1);
    if (!host.feedback_irq_out_handler)
        goto thread_clean_action;

    host.irq_out = irq_out;
    host.irq_in = irq_in;

    host.target_mutex = rt_mutex_create("host_target_sem", RT_IPC_FLAG_FIFO);
    if (!host.target_mutex)
        goto thread_clean_feedback;

    return &host;

thread_clean_feedback:
    rt_thread_delete(host.feedback_irq_out_handler);
thread_clean_action:
    rt_thread_delete(host.action_thread);

    return RT_NULL;
}

void host_delete(host_t host)
{
    rt_mutex_delete(host->target_mutex);

    if (host->feedback_irq_out_handler)
        rt_thread_delete(host->feedback_irq_out_handler);
    if (host->action_thread)
        rt_thread_delete(host->action_thread);
}
