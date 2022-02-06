#ifndef __SPS_H__
#define __SPS_H__

struct sps
{
    rt_thread_t irq_in_handler;         /**< IRQ_IN Handler */
    rt_thread_t irq_out_handler;        /**< IRQ_OUT Handler */
    rt_thread_t gpio_setting;           /**< Set GPIO Thread */
    rt_thread_t ping_thread;            /**< Ping Thread */

    rt_uint32_t irq_in;                 /**< IRQ_IN message */
    rt_uint32_t irq_out;                /**< IRQ_OUT message */
    rt_uint32_t gpio[SPS_NUM_TARGETS];  /**< GPIO message */
    rt_uint32_t ping[SPS_NUM_TARGETS];  /**< Ping message */
};

int sps_init(struct sps *sps);
int sps_start(struct sps *sps);


#endif
