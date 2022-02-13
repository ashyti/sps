#include <rtthread.h>
#include <stdio.h>

#include "sps.h"

#define SPS_PONG_COUNT   40
#define SPS_PING_PERIOD 100 /* 1 second */

struct sps sps = { };

static void ping_thread(void *param)
{
    rt_uint32_t msg_targets_status = 0;
    rt_uint8_t has_changed = 0;
    sps_t sps = param;
    rt_err_t err;
    rt_uint8_t i;

    /* Send ping */
    for (i = 0; i < SPS_NUM_TARGETS; i++)
    {
        rt_uint32_t msg = 1;

        /* target is off */
        if (!sps->targets[i])
            continue;

        /*
         * increase the reading counter that might be
         * 0-ed later if the client responds to the ping
         */
        sps->targets_pong[i]++;

        err = rt_mb_send(sps->mb_ping[i], msg);
	    if (err)
            rt_kprintf("SPS: failed to ping target %u\n", i);
    }

    /* Read pong until message queue is empty */
    while (1)
    {
        /* Set value to anything but the values we want */
        rt_ubase_t msg_targets = 0x1111;

        /* read until there is nothing in the queue */
        rt_mb_recv(sps->mb_ping_ack, &msg_targets, RT_WAITING_NO);
        if (err)
            break;

        if (msg_targets > SPS_NUM_TARGETS - 1)
            /* bad message format */
            continue;

        sps->targets_pong[msg_targets] = 0;
    }

    err = rt_mutex_take(sps->target_mutex, RT_WAITING_FOREVER);
    if (err)
    {
        rt_kprintf("SPS: failed to synchronize. Skipping\n");
        return;
    }

    for (i = 0; i < SPS_NUM_TARGETS; i++)
    {
        /* target is off */
        if (!sps->targets[i])
            continue;

        if (sps->targets_pong[i] < SPS_PONG_COUNT)
        {
            sps->targets[i] = 1;
            msg_targets_status |= 1 << i;
            continue;
        }

        /*
         * sps->targets_pong[i] >= SPS_PONG_COUNT
         * The target is known to be on but it's not responding
         * to the SPS's ping. It needs to be reported as off.
         */
        sps->targets_pong[i] = 0; /* reset */
        sps->targets[i] = 0;
        has_changed = 1;
    }

    if (has_changed)
    {
        err = rt_mb_send(sps->mb_ping_irq, msg_targets_status);
        if (err)
            rt_kprintf("SPS: failed to notify host on target status change\n");
    }

    rt_mutex_release(sps->target_mutex);
}

static void irq_out_handler(void *param)
{
    while (1)
    {
        rt_ubase_t targets;
        sps_t sps = param;
        rt_err_t err;

        /* receiving message from ping */
        err = rt_mb_recv(sps->mb_ping_irq, &targets, RT_WAITING_FOREVER);
        if (err) {
            rt_kprintf("SPS: failed to receive status from ping\n");
            continue;
        }

        /* Sending message to host  */
        rt_mb_send(sps->irq_out, targets);
        if (err)
            rt_kprintf("SPS: failed to notify host on target status change\n");
    }
}

static void irq_in_handler(void *param)
{
    sps_t sps = param;
    rt_err_t err;

    while (1)
    {
        rt_ubase_t targets;
        int i;

        err = rt_mb_recv(sps->irq_in, &targets, RT_WAITING_FOREVER);
        if (err)
        {
            rt_kprintf("SPS: failed to receive message from host. Skipping\n");
            continue;
        }

        err = rt_mutex_take(sps->target_mutex, RT_WAITING_FOREVER);
        if (err)
        {
            rt_kprintf("SPS: failed to synchronize. Skipping\n");
            continue;
        }

        for (i = 0; i < SPS_NUM_TARGETS; i++)
        {

            rt_uint8_t new_target = !!(targets & (1 << i));

            if (!(new_target ^ sps->targets[i]))
                continue;

            printf("SPS: Sending to GPIO_%d:%d\n",i,new_target);
            rt_mb_send(sps->gpio[i], new_target);
            sps->targets[i] = new_target;
        }
        rt_mutex_release(sps->target_mutex);
    }
}

rt_err_t sps_start(sps_t sps)
{
    rt_err_t err;

    err = rt_thread_startup(sps->irq_out_handler);
    if (err)
        return err;

    err = rt_thread_startup(sps->ping_thread);
    if (err)
        return err;

    return rt_thread_startup(sps->irq_in_handler);
}

static rt_err_t __sps_init_mailboxes(sps_t sps)
{
    rt_uint8_t i, j;

    sps->irq_out = rt_mb_create("irq_out", sizeof(rt_uint8_t),
                                RT_IPC_FLAG_FIFO);
    if (!sps->irq_out)
        return RT_ERROR;

    sps->irq_in = rt_mb_create("irq_in", sizeof(rt_uint8_t), RT_IPC_FLAG_FIFO);
    if (!sps->irq_in)
        goto delete_out;

    sps->mb_ping_ack = rt_mb_create("pingack", sizeof(rt_uint32_t),
                                    RT_IPC_FLAG_FIFO);
    if (!sps->mb_ping_ack)
        goto delete_in;

    sps->mb_ping_irq = rt_mb_create("pingirq", sizeof(rt_uint32_t),
                                    RT_IPC_FLAG_FIFO);
    if (!sps->mb_ping_irq)
        goto delete_ping_ack;

    for (i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        char name1[] = "pingX";
        char name2[] = "gpioX";
        char x = 48 + i; /* 48 is ascii code for '0' */

        name1[sizeof(name1) - 2] = x;
        sps->mb_ping[i] = rt_mb_create(name1, sizeof(rt_uint8_t),
                                       RT_IPC_FLAG_FIFO);
        if (!sps->mb_ping[i])
            goto mb_delete_ping;

        name2[sizeof(name2) - 2] = x;
        sps->gpio[i] = rt_mb_create(name2, sizeof(rt_uint8_t),
                                    RT_IPC_FLAG_FIFO);
        if (!sps->gpio[i]) {
            /* need to clear mb_ping[i] from exit label */
            i++;
            goto mb_delete_ping;
        }
    }

    return RT_EOK;

mb_delete_ping:
    for (j = i - 1; j >= 0; j--)
    {
            rt_mb_delete(sps->mb_ping[j]);
            /* sps->gpio[j] has not been created yet */
            if (j == i - 1)
                continue;
            rt_mb_delete(sps->gpio[j]);
    }

    rt_mb_delete(sps->mb_ping_irq);
delete_ping_ack:
    rt_mb_delete(sps->mb_ping_ack);
delete_in:
    rt_mb_delete(sps->irq_in);
delete_out:
    rt_mb_delete(sps->irq_out);

    return RT_ERROR;
}

static rt_err_t __sps_init_threads(sps_t sps)
{
    sps->irq_out_handler = rt_thread_create("sps_irq_out", irq_out_handler,
                                            sps, SPS_THREAD_STACK_SIZE, 1, 1);
    if (!sps->irq_out_handler)
        return RT_ERROR;

    sps->irq_in_handler = rt_thread_create("sps_irq_in", irq_in_handler, sps,
                                           SPS_THREAD_STACK_SIZE, 1, 1);
    if (!sps->irq_in_handler)
        goto thread_delete_out;

    sps->ping_thread = rt_thread_create_periodic("sps_ping", ping_thread, sps,
                                                 SPS_THREAD_STACK_SIZE, 1, 1,
                                                 SPS_PING_PERIOD);
    if (!sps->ping_thread)
        goto thread_delete_in;

    return RT_EOK;

thread_delete_in:
    rt_thread_delete(sps->irq_in_handler);
thread_delete_out:
    rt_thread_delete(sps->irq_out_handler);

    return RT_ERROR;
}

static void __sps_clear_threads(sps_t sps)
{
    if (sps->ping_thread)
        rt_thread_delete(sps->ping_thread);
    if (sps->irq_in_handler)
        rt_thread_delete(sps->irq_in_handler);
    if (sps->irq_out_handler)
        rt_thread_delete(sps->irq_out_handler);
}

sps_t sps_init(void)
{
    rt_err_t err;

    sps.target_mutex = rt_mutex_create("sps_target_mutex", RT_IPC_FLAG_FIFO);
    if (!sps.target_mutex)
        return RT_NULL;

    err = __sps_init_threads(&sps);
    if (err)
        goto delete_mutex;

    err = __sps_init_mailboxes(&sps);
    if (err)
        goto clear_threads;

    return &sps;

clear_threads:
    __sps_clear_threads(&sps);
delete_mutex:
    rt_mutex_delete(sps.target_mutex);

    return RT_NULL;
}

void sps_delete(sps_t sps)
{
    rt_uint8_t i;

    rt_mutex_delete(sps->target_mutex);

    for (i = 0; i < SPS_NUM_TARGETS; i++)
    {
        rt_mb_delete(sps->mb_ping[i]);
        rt_mb_delete(sps->gpio[i]);
    }

    rt_mb_delete(sps->mb_ping_irq);
    rt_mb_delete(sps->mb_ping_ack);
    rt_mb_delete(sps->irq_in);
    rt_mb_delete(sps->irq_out);

    __sps_clear_threads(sps);
}
