#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/queue.h"

static const char *TAG = "BUZZER_MODULE";
static QueueHandle_t g_buzzer_queue = NULL;

static void buzzer_signal()
{
    for (int i = 0; i < 200; i++)
    { // ~100 мс звуку
        gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 1);
        esp_rom_delay_us(250); // Частота ~2 кГц
        gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 0);
        esp_rom_delay_us(250);
    }
}

static void buzzer_task(void *pvParameters)
{
    buzzer_cmd_t current_cmd = BUZZER_CMD_STOP;

    while (1)
    {
        // Перевіряємо, чи прийшла нова команда з черги без тривалого блокування
        buzzer_cmd_t new_cmd;
        if (xQueueReceive(g_buzzer_queue, &new_cmd, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            current_cmd = new_cmd;
        }

        switch (current_cmd)
        {
        case BUZZER_CMD_PEDESTRIAN_WALK:
            buzzer_signal();
            // gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        case BUZZER_CMD_WARNING:
            buzzer_signal();
            // gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 0);
            vTaskDelay(pdMS_TO_TICKS(300));
            break;

        case BUZZER_CMD_STOP:
        default:
            gpio_set_level(CONFIG_TRAFFIC_PIN_BUZZER, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        }
    }
}

esp_err_t buzzer_init(TaskHandle_t *out_buzzer_task_handle)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_TRAFFIC_PIN_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure Buzzer GPIO %d", CONFIG_TRAFFIC_PIN_BUZZER);
        return err;
    }

    g_buzzer_queue = xQueueCreate(5, sizeof(buzzer_cmd_t));
    if (g_buzzer_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create buzzer queue");
        return ESP_FAIL;
    }

    BaseType_t res = xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 5, out_buzzer_task_handle);
    if (res != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create buzzer task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", CONFIG_TRAFFIC_PIN_BUZZER);
    return ESP_OK;
}

void buzzer_send_cmd(buzzer_cmd_t cmd)
{
    if (g_buzzer_queue != NULL)
    {
        xQueueSend(g_buzzer_queue, &cmd, 0);
    }
}

#endif