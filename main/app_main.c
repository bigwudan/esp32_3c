/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "lcd_dev.h"

#include "wifi_mod.h"
#include "bluetooth.h"
#include "spi_driver.h"

#include "esp_log.h"

static const char TAG[] = "app_main";

void app_main(void)
{
#if 0    
    wifi_mod_start();

    bluetooth_app();

    lcd_dev_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lcd_dev_task();
    }
#else
    esp_err_t ret;
    ret = spi_driver_init();

    ESP_LOGI(TAG, "spi_driver[%d]", ret);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        spi_driver_set_cs(0);
        ESP_LOGI(TAG, "rev SpiInOut[%02X]", SpiInOut(0xC0));
        spi_driver_set_cs(1);
    }    
#endif
}
