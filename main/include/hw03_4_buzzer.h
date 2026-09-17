#ifndef HW_03_4_BUZZER_H
#define HW_03_4_BUZZER_H

#include "sdkconfig.h"

#if CONFIG_HW_03_4_BUZZER_PWM
#pragma once

void hw03_4_run(void);

#endif
#endif