#ifndef TRAFFIC_FSM_H
#define TRAFFIC_FSM_H

#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "esp_err.h"

esp_err_t traffic_fsm_init(void);

#endif
#endif