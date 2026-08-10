#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "traffic_fsm.h"
#include "button.h"
#include "buzzer.h"
#include "sensor_ldr.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TRAFFIC_FSM";
static TaskHandle_t g_fsm_task_handle = NULL;

static void set_leds(bool red, bool yellow, bool green)
{
    gpio_set_level(CONFIG_TRAFFIC_PIN_LED_RED, red);
    gpio_set_level(CONFIG_TRAFFIC_PIN_LED_YELLOW, yellow);
    gpio_set_level(CONFIG_TRAFFIC_PIN_LED_GREEN, green);
}

static esp_err_t leds_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_TRAFFIC_PIN_LED_RED) |
                        (1ULL << CONFIG_TRAFFIC_PIN_LED_YELLOW) |
                        (1ULL << CONFIG_TRAFFIC_PIN_LED_GREEN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    return gpio_config(&io_conf);
}

static void traffic_fsm_task(void *pvParameters)
{
    traffic_state_t state = TRAFFIC_STATE_RED;
    bool button_pressed = false;

    while (1)
    {
        // Перевірка режиму день/ніч за LDR
        if (sensor_ldr_get_mode() == LDR_MODE_NIGHT)
        {
            state = TRAFFIC_STATE_NIGHT_FLASH;
        }

        switch (state)
        {
        case TRAFFIC_STATE_RED:
            ulTaskNotifyTake(pdTRUE, 0); // Очищуємо буфер сповіщень щоб відмінити натискання під час "зеленого" або "жовтого" світла
            set_leds(true, false, false);
            buzzer_send_cmd(BUZZER_CMD_STOP);

            // Очікуємо інтервал АБО нотифікацію від кнопки
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CONFIG_TRAFFIC_TIME_RED)) > 0)
            {
                ESP_LOGI(TAG, "Pedestrian button registered during RED");
            }
            state = TRAFFIC_STATE_RED_YELLOW;
            break;

        case TRAFFIC_STATE_RED_YELLOW:
            set_leds(true, true, false);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TRAFFIC_TIME_RED_YELLOW));
            state = TRAFFIC_STATE_GREEN;
            break;

        case TRAFFIC_STATE_GREEN:
            set_leds(false, false, true);
            buzzer_send_cmd(BUZZER_CMD_PEDESTRIAN_WALK);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TRAFFIC_TIME_GREEN));
            state = TRAFFIC_STATE_GREEN_FLASH;
            break;

        case TRAFFIC_STATE_GREEN_FLASH:
        {
            buzzer_send_cmd(BUZZER_CMD_WARNING);
            int flashes = CONFIG_TRAFFIC_TIME_GREEN_FLASH / 500;
            for (int i = 0; i < flashes; i++)
            {
                set_leds(false, false, true); // Зелений
                vTaskDelay(pdMS_TO_TICKS(250));
                set_leds(false, false, false);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            state = TRAFFIC_STATE_YELLOW;
            break;
        }

        case TRAFFIC_STATE_YELLOW:
            set_leds(false, true, false);
            buzzer_send_cmd(BUZZER_CMD_STOP);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TRAFFIC_TIME_YELLOW));
            state = TRAFFIC_STATE_RED;
            break;

        case TRAFFIC_STATE_NIGHT_FLASH:
            buzzer_send_cmd(BUZZER_CMD_STOP);
            while (sensor_ldr_get_mode() == LDR_MODE_NIGHT)
            {
                set_leds(false, true, false);
                vTaskDelay(pdMS_TO_TICKS(500));
                set_leds(false, false, false);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            state = TRAFFIC_STATE_RED;
            break;
        }
    }
}

esp_err_t traffic_fsm_init(void)
{
    ESP_ERROR_CHECK(leds_gpio_init());
    ESP_ERROR_CHECK(sensor_ldr_init());
    ESP_ERROR_CHECK(buzzer_init(NULL));

    BaseType_t res = xTaskCreate(traffic_fsm_task, "traffic_fsm_task", 3072, NULL, 5, &g_fsm_task_handle);
    if (res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create FSM task");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(button_init(g_fsm_task_handle));

    ESP_LOGI(TAG, "Traffic Light FSM fully initialized");
    return ESP_OK;
}

#endif