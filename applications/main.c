#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "host.h"
#include "sps.h"
#include "target.h"

int main(void)
{
    sps_t sps;
    host_t host;
    target_t target;
    rt_err_t ret;

    printf("hello rt-thread\n");

    sps = sps_init();
    if (!sps)
        /* handle error */ ;
    host = host_init(sps->irq_out, sps->irq_in);
    if (!host) {
        printf("Failed to create host\n");
        return RT_ERROR;
    }


    target = target_init(sps->mb_ping, sps->gpio[0]);

    ret = sps_start(sps);
    ret = host_start(host);
    ret = target_start(target);

    printf("bye rt-thread\n");

    return ret;
}
