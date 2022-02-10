#include <rtthread.h>
#include <stdio.h>

#include "sps.h"

struct sps sps = { };

void ping_thread(void *param)
{
    sps_t sps = param;
    rt_err_t err;
    rt_uint32_t msg ;
    rt_uint32_t msg_targets ;
    rt_uint8_t has_changed = 0;
    rt_uint32_t msg_targets_status;

    //Send ping
    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        msg = 1;
        rt_mb_send(sps->mb_ping[i], msg);
        //rt_kprintf("SPS: Sending ping[%d]: %d \n",i, msg);
    }


    //Read ping ack
    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        msg_targets = 0x1111;       //Setting value to anything but the values we want
        err = rt_mb_recv(sps->mb_ping_ack, &msg_targets, RT_WAITING_NO);
        sps->targets_pong[0]++;
        sps->targets_pong[1]++;
        sps->targets_pong[2]++;
        sps->targets_pong[3]++;

        if(msg_targets == 1)
        {
            sps->targets_pong[0] = 0;
        }
        if(msg_targets == 2)
        {
            sps->targets_pong[1] = 0;
        }
        if(msg_targets == 4)
        {
            sps->targets_pong[2] = 0;
        }
        if(msg_targets == 8)
        {
            sps->targets_pong[3] = 0;
        }

    }


    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        if (sps->targets_pong[i] >= PONG_COUNT)
        {
            sps->targets_pong[i] = 0 ; //reset
            sps->targets_feedback[i] = 0;
            has_changed = 1;
        }
        else
        {
            sps->targets_feedback[i] = 1;
        }
    }

    if(has_changed)
    {
        has_changed = 0;
        msg_targets_status = 0 ;
        for (rt_int8_t i = (SPS_NUM_TARGETS-1) ; i >= 0 ; i--)
        {
            msg_targets_status |= (sps->targets_feedback[i] << i);
        }
        rt_mb_send(sps->mb_ping_irq, msg_targets_status);

    }

}

void irq_out_handler(void *param)
{

    sps_t sps = param;
    rt_err_t err;

    while (1)
    {
        rt_ubase_t targets;

        //receiving message from ping
        err = rt_mb_recv(sps->mb_ping_irq, &targets, RT_WAITING_FOREVER);

        //Sending message to host
        rt_mb_send(sps->irq_out, targets);

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

    sps.ping_thread = rt_thread_create_periodic("sps_ping",
                                                 ping_thread,
                                                 &sps,
                                                 SPS_THREAD_STACK_SIZE,
                                                 1, 1, SPS_PING_PERIOD);

    sps.irq_out = rt_mb_create("irq_out", sizeof(rt_uint8_t),
                               RT_IPC_FLAG_FIFO);
    if (!sps.irq_out)
        goto thread_delete_in;

    sps.irq_in = rt_mb_create("irq_in", sizeof(rt_uint8_t),
                              RT_IPC_FLAG_FIFO);
    if (!sps.irq_in)
        goto mb_delete_out;

    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        char name1[] = "pingx";
        name1[sizeof(name1)-2] = 48 + i;
        sps.mb_ping[i] = rt_mb_create(name1, sizeof(rt_uint8_t),
                               RT_IPC_FLAG_FIFO);

        if (!sps.mb_ping[i])
            goto mb_delete_ping;

        char name2[] = "gpiox";
        name2[sizeof(name2)-2] = 48 + i;
        sps.gpio[i] = rt_mb_create(name2, sizeof(rt_uint8_t),
                                    RT_IPC_FLAG_FIFO);
    }


    sps.mb_ping_ack = rt_mb_create("pingack", sizeof(rt_uint32_t),
                               RT_IPC_FLAG_FIFO);
    if (!sps.mb_ping_ack)
        goto mb_delete_ping;

    sps.mb_ping_irq = rt_mb_create("pingirq", sizeof(rt_uint32_t),
                               RT_IPC_FLAG_FIFO);
    if (!sps.mb_ping_irq)
        goto mb_delete_ping;

    sps.target_mutex = rt_mutex_create("sps_target_mutex", RT_IPC_FLAG_FIFO);
    if (!sps.target_mutex)
        goto mb_delete_ping;

    return &sps;

mb_delete_ping:
    rt_mb_delete(sps.mb_ping);
    rt_mb_delete(sps.mb_ping_ack);
    rt_mb_delete(sps.mb_ping_irq);
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
    rt_mb_delete(sps->mb_ping_ack);
    rt_mb_delete(sps->irq_in);
    rt_mb_delete(sps->irq_out);

    if (sps->irq_in_handler)
        rt_thread_delete(sps->irq_in_handler);
    if (sps->irq_out_handler)
        rt_thread_delete(sps->irq_out_handler);
}
