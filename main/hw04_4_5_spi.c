#include "hw04_4_5_spi.h"

#if CONFIG_HW_04_4_5_SPI_MASTER

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "HW_04_4_5_SPI";

void hw04_4_5_run(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;

    // Конфігурація шини SPI (VSPI / SPI3_HOST)
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_HW_04_4_5_SPI_MISO_GPIO,
        .mosi_io_num = CONFIG_HW_04_4_5_SPI_MOSI_GPIO,
        .sclk_io_num = CONFIG_HW_04_4_5_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32 // Нам потрібно лише 12 байт
    };

    // Налаштування пристрою (STM32)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = CONFIG_HW_04_4_5_SPI_FREQ_MHZ * 1000 * 1000,
        .mode = 0, // CPOL=0, CPHA=0 (Mode 0)
        .spics_io_num = CONFIG_HW_04_4_5_SPI_CS_GPIO,
        .queue_size = 1,
    };

    // Ініціалізація шини з автоматичним вибором DMA
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // Додавання STM32 як пристрою на шину
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "SPI Master initialized. Polling STM32 every 1000 ms...");

    uint8_t tx_data[12] = {0}; // "Пусті" байти для генерації тактування
    uint8_t rx_data[12] = {0}; // Буфер для прийому (Тиск, Температура, Вологість)

    spi_transaction_t t = {
        .length = 8 * 12, // 12 байт (96 біт)
        .tx_buffer = tx_data,
        .rx_buffer = rx_data};

    while (1)
    {
        // Опитування STM32
        ret = spi_device_polling_transmit(spi, &t);

        if (ret == ESP_OK)
        {
            // Десеріалізація (STM32 передає 3 int32_t значення у форматі Little Endian)
            int32_t pressure = (rx_data[3] << 24) | (rx_data[2] << 16) | (rx_data[1] << 8) | rx_data[0];
            int32_t temperature = (rx_data[7] << 24) | (rx_data[6] << 16) | (rx_data[5] << 8) | rx_data[4];
            int32_t humidity = (rx_data[11] << 24) | (rx_data[10] << 16) | (rx_data[9] << 8) | rx_data[8];

            ESP_LOGI(TAG, "Отримано від STM32 -> Температура: %.2f C | Тиск: %.2f hPa | Вологість: %.2f %%",
                     temperature / 100.0,
                     pressure / 256.0 / 100.0, // Формат BME280
                     humidity / 1024.0);
        }
        else
        {
            ESP_LOGE(TAG, "Помилка транзакції SPI");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#endif