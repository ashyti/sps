#include <rtthread.h>
#include <stdio.h>

#include "target.h"
#include "main.h"


static void simulation_thread_entry(void* parameter)
{
    rt_err_t err;

    //Read gpio

    struct target *target = parameter;
    rt_uint32_t gpio_status;

    err = rt_mb_recv(target->mb_gpio, &gpio_status, RT_WAITING_FOREVER);
    if(err)
    {
        /* handle error */ ;
    }

    else
    {
        rt_kprintf("This is the message:%d\n", gpio_status);
        if (gpio_status == 0)
        {
            //stop and do nothing
            rt_kprintf("Stopping\n");
        }

        else{
            //Read ping
            rt_kprintf("Reading Ping\n");
        }
    }


}


rt_err_t target_init(struct target *target)
{
    printf("Initializing target\n");

    target->simulation_thread = rt_thread_create_periodic("simulation_thread_entry",
                                                    simulation_thread_entry,
                                                    target,
                                                    TARGET_STACK_SIZE,
                                                    TARGET_PRIORITY,
                                                    TARGET_TICK,
                                                    TARGET_PERIOD);

    return RT_EOK;
}


rt_err_t target_start(struct target *target)
{
    printf("Starting target`s threads\n");

    rt_thread_startup(target->simulation_thread);

    return RT_EOK;
}

