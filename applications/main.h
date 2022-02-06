#ifndef __MAIN_H__
#define __MAIN_H__

#define SPS_THREAD_STACK_SIZE   1024
#define SPS_NUM_TARGETS            4

#include "host.h"
#include "target.h"
#include "sps.h"

struct system {
    struct host host;
    struct sps sps;
    struct target target[SPS_NUM_TARGETS];
};

#define BIT(N)  1 << N
#endif
