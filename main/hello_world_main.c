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
#include "esp_log.h"

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

#include "http_mod.h"

#include "knob.h"

#include "knob_task.h"

#include "i2c_sht20.h"

#include "wifi_mod.h"


/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"



static const char *TAG = "MAIN";

   float show_temper = 0;
   float show_humidity = 0;





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
extern char test_show_buf[124];

extern char test_show_http_buf[124];

lv_obj_t *t_obj;

lv_obj_t *label_1;


lv_obj_t *label_2;

lv_obj_t *label_temper;


lv_obj_t *label_humidity;



void _my_task(){
    static int idx = 0;
    char t_buf[64] = {0};

    lv_label_set_text(label_1, test_show_buf);  // 显示数字

    lv_label_set_text(label_2, test_show_http_buf);  // 显示数字

    sprintf(t_buf, "%.2f", show_temper);
    lv_label_set_text(label_temper, t_buf);  // 显示数字

    sprintf(t_buf, "%.2f", show_humidity);
    lv_label_set_text(label_humidity, t_buf);  // 显示数字

    if(idx%2 == 0){
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 11);
        lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN); 


    }else{
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 22);
         lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN); 

    }

    idx++;

}



static void _ui_int(){

    _lv_timer_create();
    t_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(t_obj, 240,240);
    lv_obj_set_pos(t_obj, 0, 0);


    label_1 =  lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, test_show_buf);  // 显示数字
    lv_obj_center(label_1);

    label_2 =  lv_label_create(lv_scr_act());
    lv_label_set_text(label_2, test_show_http_buf);  // 显示数字
    lv_obj_set_pos(label_2, 20, 40);



    label_temper =  lv_label_create(lv_scr_act());
    
    lv_obj_set_pos(label_temper, 20, 60);
    lv_label_set_text(label_temper, "temper");  // 显示数字

    label_humidity =  lv_label_create(lv_scr_act());
    lv_label_set_text(label_temper, "humidity");  // 显示数字
    lv_obj_set_pos(label_humidity, 20, 80);


}





void app_main(void)
{
    printf("******1111*****app_main now.\n");
    printf("******1111*****app_main now.\n");
    printf("******1111*****app_main now.\n");

    esp_err_t res;
    enum knob_state knob_res; 

    res = wifi_mod_start();

    if(res != ESP_OK){
        
        ESP_LOGI(TAG, "wifi_mod_init_no_ok");

    }else{

        ESP_LOGI(TAG, "wifi_mod_init_ok");
    }

    
    while(1);
 
    //初始化i2c
    i2c_master_init();
  

    //knob_init();
    knob_task_init();

    bluetooth_app();
    fast_scan();
    lcd_dev_init();
    lv_init();
    lv_port_disp_init();

    _ui_int();
    //lv_task_create(_my_task, 500, LV_TASK_PRIO_MID, NULL);
    while(1){
	    vTaskDelay(20 / portTICK_PERIOD_MS);

        i2c_sht20_task();
        
        //knob_res = knob_get_state();
        knob_res =knob_task_get_state();
        if(knob_res != knob_still){

            if(knob_res == knob_right){

                printf("**********************************knob_right\n");
            }else{
                printf("**********************************knob_left\n");
            }
            
        }
        lv_task_handler();
        //_test_task();
        _my_task();

    }



}
