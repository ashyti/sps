#include <rtthread.h>
#include <stdio.h>

#include "sps.h"

struct sps sps = { };

void ping_thread(void *param)
{
    sps_t sps = param;
    rt_err_t err;

    rt_uint32_t msg = 1 ;

    rt_kprintf("Sending ping: %d \n", msg);

    rt_mb_send(sps->mb_ping, msg);
}

void irq_out_handler(void *param)
{
    sps_t sps = param;
    rt_err_t err;

    while (1)
    {
        rt_ubase_t targets;

        err = rt_mb_recv(sps->mb_ping, &targets, RT_WAITING_FOREVER);
        if (err)
            /* handle error */ ;
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

        rt_kprintf("%d - %s (WAITING)\n", __LINE__, __func__);

        err = rt_mb_recv(sps->irq_in, &targets, RT_WAITING_FOREVER);
        if (err)
            /* handle error */ ;

        rt_kprintf("%d - %s, %lx\n", __LINE__, __func__, targets);

        err = rt_sem_take(sps->target_sem, RT_WAITING_FOREVER);
        if (err)
            /* handle error */ ;

        rt_kprintf("%d - %s\n", __LINE__, __func__);

        for (i = 0; i < SPS_NUM_TARGETS; i++)
        {
            rt_uint8_t new_target = !!(targets | (1 << i));

            if (new_target ^ sps->targets[i])
                continue;

            rt_mb_send(sps->gpio[i], new_target);
            sps->targets[i] = new_target;
        }
        rt_kprintf("%d - %s\n", __LINE__, __func__);
        rt_sem_release(sps->target_sem);
    }
}

rt_err_t sps_start(sps_t sps)
{
    rt_thread_startup(sps->irq_out_handler);
    rt_thread_startup(sps->irq_in_handler);
    rt_thread_startup(sps->ping_thread);
    return RT_EOK;
}

sps_t sps_init(void)
{
    //int i;

    sps.irq_out_handler = rt_thread_create("sps_irq_out", irq_out_handler,
                                           &sps, SPS_THREAD_STACK_SIZE, 1, 1);
    sps.irq_in_handler = rt_thread_create("sps_irq_in", irq_in_handler, &sps,
                                          SPS_THREAD_STACK_SIZE, 1, 1);

    sps.ping_thread = rt_thread_create_periodic("sps_ping",
                                                 ping_thread,
                                                 &sps,
                                                 SPS_THREAD_STACK_SIZE,
                                                 1,
                                                 1,
                                                 100);

    /*
    sps->irq_gpio_handler = rt_thread_create("sps_gpio_set",
                                             gpio_set, sps, 1, 1);
    */

    sps.irq_out = rt_mb_create("irq_out", sizeof(rt_uint8_t),
                                RT_IPC_FLAG_FIFO);
    sps.irq_in = rt_mb_create("irq_in", sizeof(rt_uint8_t),
                               RT_IPC_FLAG_FIFO);
    sps.mb_ping = rt_mb_create("mb_ping", sizeof(rt_uint8_t),
                                RT_IPC_FLAG_FIFO);

    sps.gpio[0] = rt_mb_create("mb_gpio", sizeof(rt_uint8_t),
                                RT_IPC_FLAG_FIFO);

#if 0
    for (i = 0; i < SPS_NUM_TARGETS; i++)
    {
        /* add number at the end of the name */
        sps->gpio[i] = rt_mb_create("gpio", sizeof(rt_uint8_t),
                                    RT_IPC_FLAG_FIFO);
        sps->ping[i] = rt_mb_create("ping", sizeof(rt_uint8_t),
                                    RT_IPC_FLAG_FIFO);
    }
#endif

    sps.target_sem = rt_sem_create("sps_target_sem", 0, RT_IPC_FLAG_FIFO);
    if (!sps.target_sem)
        return RT_NULL;

    return &sps;
}
