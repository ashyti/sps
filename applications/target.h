#ifndef __TARGET_H__
#define __TARGET_H__

struct target {
    rt_thread_t simulation_thread;

    rt_messagequeue_t gpio;
    rt_messagequeue_t ping;
};

#endif
