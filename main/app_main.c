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
#include "driver/gpio.h"

static const char TAG[] = "app_main";

static void _testio(){
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<7);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(7, 1);

}


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

    _testio();

 

    ESP_LOGI(TAG, "spi_driver[%d]", ret);
    uint8_t idx =0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        //SPI_SendData8(0x11);
    
        ESP_LOGI(TAG, "rec[0x%02X]", SpiInOut(idx++));
    }    
#endif
}
