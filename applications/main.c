
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
    //target_t target[SPS_NUM_TARGETS] = {};
    extern struct target target[SPS_NUM_TARGETS];
    rt_err_t ret;

    printf("Welcome to SPS!\n");

    sps = sps_init();
    if (!sps) {
        printf("Failed to create SPS\n");
        return RT_ERROR;
    }

    host = host_init(sps->irq_out, sps->irq_in);
    if (!host) {
        printf("Failed to create host\n");
        ret = RT_ERROR;
        goto clean_sps;
    }

    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        ret = target_init(&target[i], sps->mb_ping[i], sps->mb_ping_ack, sps->gpio[i], i);
        if (ret)
        {
            printf("Failed to start Target[%d]", i);
            goto error_exit;
        }
    }


    ret = sps_start(sps);
    if (ret) {
        printf("Failed to start SPS");
        goto error_exit;
    }

    ret = host_start(host);
    if (ret) {
        printf("Failed to start host");
        goto error_exit;
    }

    for (rt_uint8_t i = 0 ; i < SPS_NUM_TARGETS ; i++)
    {
        ret = target_start(&target[i]);
        if (ret)
        {
            printf("Failed to start target");
            goto error_exit;
        }
    }

    return RT_EOK;

error_exit:
    host_delete(host);
clean_sps:
    sps_delete(sps);

    return ret;
}
