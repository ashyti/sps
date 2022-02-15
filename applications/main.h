#ifndef __MAIN_H__
#define __MAIN_H__

#include <rtdef.h>

#define SPS_THREAD_STACK_SIZE   1024
#define SPS_NUM_TARGETS           32

#define bool _Bool

#ifndef false
  #define false 0
#endif

#ifndef true
  #define true 1
#endif

enum sps_pwr_status
{
    SPS_OFF,
    SPS_ON,
    SPS_FROZEN,
};

#ifndef min
#define min(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a < _b ? _a : _b; })
#endif

#endif
