/* Hello World Example

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

#include "knob.h"

//防抖时间
#define EXIT_SHARE_TIME 0

//间隔多久上报一次
#define DURING_TIEM 8

#define GPIO_INPUT_IO_0     0
#define GPIO_INPUT_IO_1     1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

typedef struct {
   uint8_t idx; //0未开始 1第一次  2第二次
   uint8_t input_level; //0低  1高


}knob_node_t;

typedef enum knob_chg_state
{
    wait_one,
    wait_two,
};


struct knob_info{
   knob_node_t old_a;
   knob_node_t old_b;
   enum knob_chg_state state;
   uint32_t tick; 
}knob_data;

typedef struct{
   uint8_t port_io;
   uint8_t input_level; //0低  1高
   uint32_t tick; //tick时间

}msg_data_t;


static xQueueHandle gpio_evt_queue = NULL;





static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    static uint32_t _old_tick = 0;
   uint32_t tick = xTaskGetTickCount();
   if(_old_tick != tick){
      msg_data_t msg_data = {0};
      msg_data.port_io = gpio_num;
      msg_data.input_level = gpio_get_level(gpio_num);
      msg_data.tick = tick;
      xQueueSendFromISR(gpio_evt_queue, &msg_data, NULL);
   }
   _old_tick = tick;

}



void knob_init(){
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
    gpio_evt_queue = xQueueCreate(10, sizeof(msg_data_t));
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

   knob_data.old_a.input_level = gpio_get_level(GPIO_INPUT_IO_0);
   knob_data.old_b.input_level = gpio_get_level(GPIO_INPUT_IO_1);


   printf("************ a[%d]b[%d]\n", knob_data.old_a.input_level, knob_data.old_b.input_level);
}


/*
更新历史状态

*/
static inline void _update_old(msg_data_t *msg_data, uint8_t state){
   if(msg_data->port_io == GPIO_INPUT_IO_0){
      knob_data.old_a.input_level = msg_data->input_level;

   }else{

      knob_data.old_b.input_level = msg_data->input_level;
   }
   if(state == 1){
      knob_data.tick = msg_data->tick;

   }


}

//判断
static enum knob_state _check_state(msg_data_t *msg_data){
   enum knob_state res;
   //历史AB是否同一方向
   if(knob_data.old_a.input_level == knob_data.old_b.input_level){
      if(msg_data->port_io == GPIO_INPUT_IO_0 ){
         res = knob_right;
      }else{
         res = knob_left;
      }
   }else{
      if(msg_data->port_io == GPIO_INPUT_IO_0 ){
         res = knob_right;
      }else{
         res = knob_left;
      }
   }
   printf("check a[%d]b[%d]-[%d]\n", knob_data.old_a.input_level,knob_data.old_b.input_level , msg_data->port_io);
   return res;
} 


enum knob_state knob_get_state(){
   static uint32_t old_tick = 0;
   static int old_io = -1;
   static int old_io_level = -1;
   uint32_t cur_tick = 0;
   int flag = 0;
   enum knob_state res =knob_still;
   msg_data_t msg_data = {0};

   cur_tick = xTaskGetTickCount();

   flag = xQueueReceive(gpio_evt_queue, &msg_data, NULL);
   if(flag >0){
      printf("io[%d][%d][%d]\n", msg_data.port_io, msg_data.input_level, msg_data.tick);

      if(knob_data.state == wait_one){
         res =_check_state(&msg_data);
         knob_data.state = wait_two;
      }else{
         knob_data.state = wait_one;
      }
      _update_old(&msg_data ,1);

 
#if 0      
      //去抖
      if(knob_data.tick +EXIT_SHARE_TIME < msg_data.tick ){
         if(knob_data.state == wait_one){
            res =_check_state(&msg_data);
            knob_data.state = wait_two;
         }else{
            knob_data.state = wait_one;
         }
         knob_data.tick = 0;
         _update_old(&msg_data ,1);
      }else{
         //是抖动，只更新状态不更新时间
         _update_old(&msg_data ,0);
         printf("share.....\n");
      }
   
#endif
      //除去短时间上报
      if(res != knob_still){
         //printf("old=[%d],new[%d]\n",old_tick + DURING_TIEM,cur_tick );
         if((old_tick + DURING_TIEM)  <= cur_tick ){
            
            old_tick = cur_tick;
         }else{

            res = knob_still;
            
         }

      }

   }
   return res;
}


