/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "knob_task.h"

#include "driver/timer.h"


//#include "EncoderScan.h"
//#include "SWM320.h"

//u8  tstVar ;

//间隔多久上报一次
#define DURING_TIEM 8

#define GPIO_INPUT_IO_0     0
#define GPIO_INPUT_IO_1     1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0



uint8_t xcnt1;    //编码器1的超时计数器
uint8_t ecdRcnt1; //右旋相位计数器
uint8_t ecdLcnt1; //左旋相位计数器

static xQueueHandle gpio_evt_queue = NULL;


typedef struct{
   uint8_t port_io;
   uint8_t input_level; //0低  1高
   uint32_t tick; //tick时间

}msg_data_t;


static void _tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec);

//u8 encodeEvent;		//此变量用来存储编码器的动作，详见 ENC_1_R 等相关定义
//在检测到编码器动作时，将此参数传给应用层， 然后立即清零此变量。

//-------------------------------------------------
// 编码器信号超时处理程序， 应保证0.1秒调用一次
// 如果系统允许， 建议每隔10ms 调用一次，然后修改超时的次数，用来实现更短的超时控制
void EncodeScanTimeOutCtrl(void)
{
    if (++xcnt1 >= 5)
    { // 5*0.1 = 0.5秒钟内没有旋转编码器则视作超时。
        xcnt1 = 0;
        ecdRcnt1 = 0;
        ecdLcnt1 = 0;
    }
   
}


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
   msg_data_t msg = {0};
   
    uint32_t gpio_num = (uint32_t) arg;
    uint8_t res = 0;

    static uint32_t _old_tick = 0;
   uint32_t tick = xTaskGetTickCount();



    uint8_t a_level = gpio_get_level(GPIO_INPUT_IO_0);
    uint8_t b_level = gpio_get_level(GPIO_INPUT_IO_1);
    res = EncoderScan(a_level, b_level);
    if(res > 0){
        msg.port_io = res;
        msg.tick = tick;
        xQueueSendFromISR(gpio_evt_queue, &msg, NULL);
    }
  
   _old_tick = tick;
    



}

void knob_task_init(){
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


    _tg_timer_init(TIMER_GROUP_0, TIMER_0, true, 100);

  
}


/*
此程序负责对1号编码器的硬件信号进行扫描
由 EncoderScan() 程序负责调用此程序

传入参数: u8 pin  其最低两位分别代表编码器管脚的高低电平状态

*/
uint8_t EncoderPinScan1(uint8_t pin)
{
    uint8_t var;
    static uint8_t lstVar = 0x03; //用来记录上一次的端口状态。

    uint8_t x = pin & 0x03; //读取最低两位(即编码器的AB两路电平状态)
    uint8_t res = 0;        //encodeEvent;	//

    var = lstVar << 2;
    var |= x;
    //printf("EncoderPinScan1: pin 0x%x, var 0x%x\n", pin, var);

    switch (var)
    {
    case EC_S1:
        ecdRcnt1 |= BIT0;
        //printf("--->EC_S1\n");
        break;
    case EC_S2:
        ecdRcnt1 |= BIT1;
        //printf("--->EC_S2\n");
        if (ecdRcnt1 == 0x03)
        { //顺时针检测到半个周期

            res &= ~ENC_1_MSK;
            res |= ENC_1_L;
            //encodeEvent = res; //此处产生右旋事件
        }
        ecdRcnt1 = 0;
        ecdLcnt1 = 0;
        break;
    case EC_S3:
        ecdRcnt1 |= BIT2;
        //printf("--->EC_S3\n");
        break;
    case EC_S4:
        ecdRcnt1 |= BIT3;
        //printf("--->EC_S4\n");
        if (ecdRcnt1 == 0x0C)
        { //顺时针检测到半个周期

            res &= ~ENC_1_MSK;
            res |= ENC_1_L;
            //encodeEvent = res; //此处产生右旋事件
        }
        ecdRcnt1 = 0;
        ecdLcnt1 = 0;
        break;
        //---------------------------------------------------------

    case EC_N1:
        ecdLcnt1 |= BIT4;
        //printf("--->EC_N1\n");
        break;
    case EC_N2:
        ecdLcnt1 |= BIT5;
        //printf("--->EC_N2\n");
        if (ecdLcnt1 == 0x30)
        { //逆时针检测到半个周期
            res &= ~ENC_1_MSK;
            res |= ENC_1_R;
            //encodeEvent = res;	//此处产生左旋事件
        }
        ecdRcnt1 = 0;
        ecdLcnt1 = 0;
        break;
    case EC_N3:
        ecdLcnt1 |= BIT6;
        //printf("--->EC_N3\n");
        break;
    case EC_N4:
        ecdLcnt1 |= BIT7;
        //printf("--->EC_N4\n");
        if (ecdLcnt1 == 0xC0)
        { //逆时针检测到半个周期
            res &= ~ENC_1_MSK;
            res |= ENC_1_R;
            //encodeEvent = res;	//此处产生左旋事件
        }
        ecdRcnt1 = 0;
        ecdLcnt1 = 0;
        break;
    default:
        break;
    }

    xcnt1 = 0;
    lstVar = x;
    return res;
}

uint8_t encodeV1 = 0;
uint8_t encodeV2 = 0;
//--------------------------------------------------------
//此程序应在编码器的AB两路信号中任何一个发生变化时 调用
//如果AB两路信号都能产生双边沿中断，则两个中断程序里都调用此程序
uint8_t EncoderScan(unsigned int gpio_A, unsigned int gpio_B)
{
    uint8_t x, px;
    //u8 px;
    uint8_t res = 0;
    //volatile u8 pin0=0;

    //-----------------------------------------------------------------
    // 这里是伪代码，需要根据具体的CPU，替换成相应的读取端口的语句
    //    pin0 = GPIO_GET_PIN(PORTA);
    //-------------------------------------------------
    encodeV1 = (uint8_t)((gpio_A << 1) | gpio_B);
    //px = (u8)((gpio_A<<1) | gpio_B);

    //-----------------------------------
    x = encodeV1 ^ encodeV2;

    if (x & (PIN_ENC1A | PIN_ENC1B))
    {
        px = (encodeV1 & PIN_ENC1A);
        px |= (encodeV1 & PIN_ENC1B);
        res = EncoderPinScan1(px);
    }

    encodeV2 = encodeV1;
    //--------------------------
    return res;
}


enum knob_state knob_task_get_state(){
   int flag = 0;
   enum knob_state res = knob_still;
   static uint32_t old_tick = 0;
   uint32_t cur_tick = 0;
   msg_data_t msg = {0};

   cur_tick = xTaskGetTickCount();

   flag = xQueueReceive(gpio_evt_queue, &msg, NULL);
   if(flag > 0){
        res = msg.port_io;


        //除去短时间上报
        if(res != knob_still){

            if((old_tick + DURING_TIEM)  <= cur_tick ){
            
                old_tick = cur_tick;
            }else{

                res = knob_still;
            
            }

        }



   }




   return res;
}


static bool IRAM_ATTR timer_group_isr_callback(void *args)
{

    EncodeScanTimeOutCtrl();
    return pdTRUE; // return whether we need to yield at the end of ISR
}

/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
static void _tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER / 1000)  // convert counter value to seconds

    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(group, timer);


    timer_isr_callback_add(group, timer, timer_group_isr_callback, NULL, 0);

    timer_start(group, timer);
}