/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "wifi_mod.h"

#include "mqtt_mod.h"

static const char *TAG = "main_";

void app_main(void)
{
   
    esp_err_t res;


    res = wifi_mod_start();

    if(res != ESP_OK){
        
        ESP_LOGI(TAG, "wifi_mod_init_no_ok");

    }else{

        ESP_LOGI(TAG, "wifi_mod_init_ok");
    }

    if(res == ESP_OK){

        mqtt_mod_start();
    }
    while(1){
	    vTaskDelay(20 / portTICK_PERIOD_MS);


    }
}
