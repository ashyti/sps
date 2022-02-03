#include <rtthread.h>

#define THREAD_STACK_SIZE    512
#define THREAD_PRIORITY      10
#define THREAD_TIMESLICE     5
#define THREAD_PERIOD        500

/* Mailbox control block */
static struct rt_mailbox mb;
/* Memory pool for mails storage */
static char mb_pool[128];

struct gpio_struct
{
    rt_uint8_t gpio;
    rt_bool_t status;
};

struct gpio_struct gpio_1 = {1, RT_TRUE};
struct gpio_struct gpio_2 = {2, RT_FALSE};


void sps_set_gpio(void* parameter)
{
    struct gpio_struct *q;
    q = (struct gpio_struct *)parameter;

    rt_uint8_t gpio = (q->gpio) << 1;
    rt_uint8_t status = (q->status) & 0x01;
    rt_uint32_t msg = (rt_uint32_t)(gpio | status);

    rt_kprintf("Sending GPIO:%d , Status: %d\n",q->gpio,q->status);
    /* Send the message to the mailbox */
    rt_mb_send(&mb, msg);
}

void sps_read_gpio(void* parameter)
{
    rt_uint32_t msg;
    if (rt_mb_recv(&mb, &msg, RT_WAITING_FOREVER) == RT_EOK)
    {
        rt_uint8_t status = msg & 0x01;
        rt_uint8_t gpio = msg >> 1;
        rt_kprintf("Receiving GPIO:%d , Status: %d\n",gpio,status);

    }
}

ALIGN(RT_ALIGN_SIZE)
static char sps_set_gpio_thread_stack[1024];
static char sps_read_gpio_thread_stack[1024];

static struct rt_thread sps_set_gpio1_thread;
static struct rt_thread sps_set_gpio2_thread;

static rt_thread_t sps_read_gpio1_thread;

int mailbox_test(void)
{
    rt_err_t result;

    /* Initialize a mailbox */
    result = rt_mb_init(&mb,
                        "mbt",                      /* Name is mbt */
                        &mb_pool[0],                /* The memory pool used by the mailbox is mb_pool */
                        sizeof(mb_pool) / 4,        /* The number of messages in the mailbox because a message occupies 4 bytes */
                        RT_IPC_FLAG_FIFO);          /* Thread waiting in FIFO approach */

    if (result != RT_EOK)
    {
        rt_kprintf("Init mailbox failed.\n");
        return -1;
    }

    //initialize thread
    rt_thread_init(&sps_set_gpio1_thread,
                   "sps_set_gpio1 thread",
                   sps_set_gpio,
                   &gpio_1,
                   &sps_set_gpio_thread_stack[0],
                   sizeof(sps_set_gpio_thread_stack),
                   THREAD_PRIORITY,
                   THREAD_TIMESLICE);

    rt_thread_startup(&sps_set_gpio1_thread);

    rt_thread_init(&sps_set_gpio2_thread,
                   "sps_set_gpio2 thread",
                   sps_set_gpio,
                   &gpio_2,
                   &sps_set_gpio_thread_stack[0],
                   sizeof(sps_set_gpio_thread_stack),
                   THREAD_PRIORITY,
                   THREAD_TIMESLICE);

    rt_thread_startup(&sps_set_gpio2_thread);

    sps_read_gpio1_thread = rt_thread_create_periodic("sps_read_gpio1_thread",
                   sps_read_gpio,
                   RT_NULL,
                   THREAD_STACK_SIZE,
                   THREAD_PRIORITY,
                   THREAD_TIMESLICE,
                   THREAD_PERIOD);

    rt_thread_startup(sps_read_gpio1_thread);


    return 0;
}

/* Export to the msh command list */
MSH_CMD_EXPORT(mailbox_test, mailbox test);



