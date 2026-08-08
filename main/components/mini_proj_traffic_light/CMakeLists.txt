idf_component_register(SRCS "src/traffic_fsm.c"
                            "src/buzzer.c"
                            "src/sensor_ldr.c"
                            "src/button.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_timer freertos driver esp_driver_gpio esp_adc)
                       