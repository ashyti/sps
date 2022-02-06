
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

    target = target_init(sps->mb_ping, sps->gpio[0]);
    if (!target) {
        printf("Failed to create targets\n");
        ret = RT_ERROR;
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
    ret = target_start(target);

    if (ret) {
        printf("Failed to start target");
        goto error_exit;
    }

    return RT_EOK;

error_exit:
    host_delete(host);
clean_sps:
    sps_delete(sps);

    return ret;
}
