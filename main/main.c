#include <stdio.h>
#include "sdkconfig.h"

#include "hw02_3_superloop.h"
#include "hw02_4_1_hw_interrupts.h"
#include "hw02_4_2_hw_interrupts.h"
#include "hw02_4_3_hw_interrupts.h"
#include "hw02_4_4_hw_interrupts.h"

void app_main(void)
{
#if CONFIG_HW_02_3_SUPERLOOP
    hw02_3_superloop_run();
#elif CONFIG_HW_02_4_1_HW_INTERRUPTS
    hw02_4_1_run();
#elif CONFIG_HW_02_4_2_HW_INTERRUPTS
    hw02_4_2_run();
#elif CONFIG_HW_02_4_3_HW_INTERRUPTS
    hw02_4_3_run();
#elif CONFIG_HW_02_4_4_HW_INTERRUPTS
    hw02_4_4_run();
#endif
}