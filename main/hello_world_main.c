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



#define GPIO_INPUT_IO_0     0
#define GPIO_INPUT_IO_1     1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}



static void _set_knob(){
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    //xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


}




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

void _my_task(){
    static int idx = 0;
    lv_label_set_text(label_1, test_show_buf);  // 显示数字

    lv_label_set_text(label_2, test_show_http_buf);  // 显示数字

    if(idx%2 == 0){
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 11);
 lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN); 


    }else{
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 22);
         lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN); 

    }

    idx++;

}


static void _run_bg(){


    gpio_pad_select_gpio(3);                 // 选择GPIO口
    gpio_set_direction(3, GPIO_MODE_OUTPUT); // GPIO作为输出
    gpio_set_level(3, 0);                    // 默认低电平



}





void app_main(void)
{
    printf("***********app_main now.\n");
    printf("***********app_main now.\n");
    printf("***********app_main now.\n");



    uint32_t io_num;
    _set_knob();
    bluetooth_app();
    fast_scan();
    lcd_dev_init();
    lv_init();
    lv_port_disp_init();
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

    //lv_task_create(_my_task, 500, LV_TASK_PRIO_MID, NULL);
    while(1){
	    vTaskDelay(20 / portTICK_PERIOD_MS);
        if(xQueueReceive(gpio_evt_queue, &io_num, 0)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        } 
        lv_task_handler();
        //_test_task();
        _my_task();

    }



}
#endif