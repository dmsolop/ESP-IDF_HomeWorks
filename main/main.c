#include <stdio.h>
#include "sdkconfig.h"

#include "hw02_3_superloop.h"

void app_main(void)
{
#if CONFIG_HW_02_SUPERLOOP
    hw02_3_superloop_run();
#elif CONFIG_HW_02_3_INTERRUPTS
    hw02_3_interrupts_run();
#endif
}