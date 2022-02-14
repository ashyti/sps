#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "sps.h"

int main(void)
{
    sps_t sps;
    rt_err_t ret;

    printf("Welcome to SPS!\n");

    /* Initialize seed for random numbers */
    srand(time(0));

    sps = sps_init();
    if (!sps) {
        printf("Failed to create SPS\n");
        return RT_ERROR;
    }


    ret = sps_start(sps);
    if (ret) {
        printf("Failed to start SPS");
        goto error_exit;
    }

    return RT_EOK;

error_exit:
    sps_delete(sps);

    return ret;
}
