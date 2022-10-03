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


void app_main(void)
{
  

    lcd_dev_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lcd_dev_task();
    } 
}
