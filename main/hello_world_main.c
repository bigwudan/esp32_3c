/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This demo showcases creating a GATT database using a predefined attribute table.
* It acts as a GATT server and can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server_service_table demo.
* Client demo will enable GATT server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"


#include "esp_spi_flash.h"

//spi lcd
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "lcd_dev.h"
#include <string.h>
#include <stdio.h>

#include "lvgl.h"
#include "../components/lvgl/lvgl_driver/lv_port_disp.h"

#include "bluetooth.h"


static esp_timer_handle_t lvgl_timer_handle = NULL;
static IRAM_ATTR void lv_timer_cb(void *arg){
    lv_tick_inc(1);
}

static esp_timer_create_args_t lvgl_timer = {
    .callback = &lv_timer_cb,
    .arg = NULL,
    .name = "lvgl_timer",
    .dispatch_method = ESP_TIMER_TASK

};

void _lv_timer_create(void){

    esp_err_t err = esp_timer_create(&lvgl_timer, &lvgl_timer_handle);
    err = esp_timer_start_periodic(lvgl_timer_handle, 1000);
}

lv_obj_t *t_obj;
void _my_task(){
    static int idx = 0;
    if(idx%2 == 0){
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 11);
 lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN); 


    }else{
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 22);
         lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN); 

    }

    idx++;

}

void app_main(void)
{
 

    printf("***********app_main now.\n");
    printf("***********app_main now.\n");
    printf("***********app_main now.\n");

    bluetooth_app();
    lcd_dev_init();




    lv_init();
    lv_port_disp_init();
    _lv_timer_create();



    t_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(t_obj, 240,240);
    lv_obj_set_pos(t_obj, 0, 0);
    //lv_task_create(_my_task, 500, LV_TASK_PRIO_MID, NULL);


    while(1){
	    vTaskDelay(2000 / portTICK_PERIOD_MS);
        lv_task_handler();
        //_test_task();
        _my_task();

    }



}
