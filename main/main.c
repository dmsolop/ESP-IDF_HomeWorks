#include <stdio.h>
#include "sdkconfig.h"

#include "hw02_3_superloop.h"
#include "hw02_4_1_hw_interrupts.h"
#include "hw02_4_2_hw_interrupts.h"
#include "hw02_4_3_hw_interrupts.h"
#include "hw02_4_4_hw_interrupts.h"
#include "hw02_5_exhaust_fan.h"
#include "hw03_1_adc_calib.h"
#include "hw03_2_adc_ema.h"
#include "traffic_fsm.h"
#include "cw_servo_pot.h"

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
#elif CONFIG_HW_02_5_EXHAUST_FAN
    exhaust_fan_init();
#elif CONFIG_MINI_PROJ_TRAFFIC_LIGHT
    traffic_fsm_init();
#elif CONFIG_HW_03_1_ADC_CALIB
    hw03_1_run();
#elif CONFIG_HW_03_2_ADC_EMA
    hw03_2_run();
#elif CONFIG_CW_SERVO_POT
    cw_servo_pot_run();
#endif
}