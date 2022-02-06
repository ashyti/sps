#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "host.h"
#include "target.h"
#include "sps.h"

int main(void)
{
    int ret;
    struct system system;

    printf("hello rt-thread\n");
/*
    ret = host_init(&system.host);

    ret = sps_init(&system.sps);
 */

    rt_mb_init(system.target[0].mb_gpio,
                "GPIO_mailbox",
                system.target[0].mb_gpio_pool,                /* The memory pool used by the mailbox is mb_pool */
                sizeof(system.target[0].mb_gpio_pool) / 4,        /* The number of messages in the mailbox because a message occupies 4 bytes */
                RT_IPC_FLAG_FIFO);


    ret = target_init(&system.target[0]);
    ret = target_start(&system.target[0]);

    rt_mb_send(system.target[0].mb_gpio, 0);

    rt_mb_send(system.target[0].mb_gpio, 1);


    return ret;
}
