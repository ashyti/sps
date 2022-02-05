#include <rtthread.h>
#include <stdio.h>

#include "sps.h"

struct sps sps = { };

void irq_out_handler(void *param)
{
    sps_t sps = param;
    rt_err_t err;

    while (1)
    {
        rt_ubase_t targets;
        int i;

        err = rt_mb_recv(sps->mb_ping, &targets, RT_WAITING_FOREVER);
        if (err)
        {
            rt_kprintf("SPS: failed to receive message from ping. Skipping\n");
            continue;
        }

        err = rt_mutex_take(sps->target_mutex, RT_WAITING_FOREVER);
        if (err)
        {
            rt_kprintf("Host: failed to synchronise. Skipping\n");
            continue;
        }

        for (i = 0; i < SPS_NUM_TARGETS; i++)
        {
            rt_uint8_t current_target = (targets >> i) & 0x1;

            sps->targets[i] = !!current_target;
        }
        rt_mutex_release(sps->target_mutex);
    }
}

void irq_in_handler(void *param)
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
            rt_kprintf("SPS: failed to synchronise. Skipping\n");
            continue;
        }

        for (i = 0; i < SPS_NUM_TARGETS; i++)
        {
            rt_uint8_t new_target = !!(targets | (1 << i));

            if (new_target ^ sps->targets[i])
                continue;

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

    return rt_thread_startup(sps->irq_in_handler);
}

sps_t sps_init(void)
{
    sps.irq_out_handler = rt_thread_create("sps_irq_out", irq_out_handler,
                                           &sps, SPS_THREAD_STACK_SIZE, 1, 1);
    if (!sps.irq_out_handler)
        return RT_NULL;

    sps.irq_in_handler = rt_thread_create("sps_irq_in", irq_in_handler, &sps,
                                          SPS_THREAD_STACK_SIZE, 1, 1);
    if (!sps.irq_in_handler)
        goto thread_delete_out;

    sps.irq_out = rt_mb_create("irq_out", sizeof(rt_uint8_t),
                               RT_IPC_FLAG_FIFO);
    if (!sps.irq_out)
        goto thread_delete_in;

    sps.irq_in = rt_mb_create("irq_in", sizeof(rt_uint8_t),
                              RT_IPC_FLAG_FIFO);
    if (!sps.irq_in)
        goto mb_delete_out;

    sps.mb_ping = rt_mb_create("mb_ping", sizeof(rt_uint8_t),
                               RT_IPC_FLAG_FIFO);
    if (!sps.mb_ping)
        goto mb_delete_in;

    sps.target_mutex = rt_mutex_create("sps_target_mutex", RT_IPC_FLAG_FIFO);
    if (!sps.target_mutex)
        goto mb_delete_ping;

    return &sps;

mb_delete_ping:
    rt_mb_delete(sps.mb_ping);
mb_delete_in:
    rt_mb_delete(sps.irq_in);
mb_delete_out:
    rt_mb_delete(sps.irq_out);
thread_delete_in:
    rt_thread_delete(sps.irq_in_handler);
thread_delete_out:
    rt_thread_delete(sps.irq_out_handler);

    return RT_NULL;
}

void sps_delete(sps_t sps)
{
    rt_mutex_delete(sps->target_mutex);

    rt_mb_delete(sps->mb_ping);
    rt_mb_delete(sps->irq_in);
    rt_mb_delete(sps->irq_out);

    if (sps->irq_in_handler)
        rt_thread_delete(sps->irq_in_handler);
    if (sps->irq_out_handler)
        rt_thread_delete(sps->irq_out_handler);
}
