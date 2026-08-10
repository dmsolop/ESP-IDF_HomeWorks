#ifndef TRAFFIC_FSM_H
#define TRAFFIC_FSM_H

#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "esp_err.h"

typedef enum
{
    TRAFFIC_STATE_RED = 0,
    TRAFFIC_STATE_RED_YELLOW,
    TRAFFIC_STATE_GREEN,
    TRAFFIC_STATE_GREEN_FLASH,
    TRAFFIC_STATE_YELLOW,
    TRAFFIC_STATE_NIGHT_FLASH
} traffic_state_t;

esp_err_t traffic_fsm_init(void);

#endif
#endif