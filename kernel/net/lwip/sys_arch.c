#include "lwip/sys.h"

#include "time/time.h"

u32_t sys_now(void)
{
    return time_millis();
}
