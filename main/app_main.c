/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "lcd_dev.h"
#include "esp_sleep.h"
#include "wifi_mod.h"
#include "bluetooth.h"
#include "spi_driver.h"

#include "radio.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "crc.h"

#include "lora_app.h"

#include <time.h>
#include <sys/time.h>
static const char TAG[] = "app_main";


int show_time(void)
{
    // time_t t;
    // struct tm *gmt, *area;
    // tzset(); /* tzset()设置时区*/
    // t = time(NULL);
    // area = localtime(&t);
    // printf("Local time is: %s", asctime(area));
    // gmt = gmtime(&t);
    // printf("GMT is: %s", asctime(gmt));
    return 0;
}

void show_task(){
    // time_t t =0;
    // struct tm *gmt;    
    // gmt = gmtime(&t);
    // printf("GMT is: %s", asctime(gmt));
    struct timeval ts_now;
    gettimeofday(&ts_now, NULL);
    //ESP_LOGI(TAG, "[%lds][%lds]", ts_now.tv_sec, ts_now.tv_usec);
    //printf("[%lld][%ld]", ts_now.tv_sec, ts_now.tv_usec);
    ESP_LOGI(TAG,"[%lld][%ld]", ts_now.tv_sec, ts_now.tv_usec);
}

static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

static void oneshot_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) arg;
    /* To start the timer which is running, need to stop it first */
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, 1000000));
    time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Restarted periodic timer with 1s period, time since boot: %lld us",
            time_since_boot);
}


//开启定时器
static void _create_alaram_time(){

    /* Create two timers:
     * 1. a periodic timer which will run every 0.5s, and print a message
     * 2. a one-shot timer which will fire after 5s, and re-start periodic
     *    timer with period of 1s.
     */

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &oneshot_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg = (void*) periodic_timer,
            .name = "one-shot"
    };
    esp_timer_handle_t oneshot_timer;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 500000));
    ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 5000000));
  
    while(1){

       vTaskDelay(pdMS_TO_TICKS(50)); //延迟500ms
    }
}


void app_main(void)
{
    //_create_alaram_time();

#if 0    
    wifi_mod_start();

    bluetooth_app();

    lcd_dev_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lcd_dev_task();
    }
#else

#if 0
    esp_err_t ret;
    ret = spi_driver_init();
    ESP_LOGI(TAG, "spi_driver[%d]", ret);


    //Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
  //  Radio.WriteBuffer(0x06C0,data,2);
   // Radio.ReadBuffer(0x06C0,test,2);
    
#if defined( USE_MODEM_LORA )
    
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
    
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, false );
    
#elif defined( USE_MODEM_FSK )
    
    Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                                  FSK_DATARATE, 0,
                                  FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, 0, 3000 );
    
    Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                                  0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                                  0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                  0, 0,false, false );
#else
    #error "Please define a frequency band in the compiler options."
#endif

    
    if(EnableMaster)
    {
          TX_Buffer[0] = 'P';
          TX_Buffer[1] = 'I';
          TX_Buffer[2] = 'N';
          TX_Buffer[3] = 'G'; 
          
          crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
          TX_Buffer[4]=crc_value>>8;
          TX_Buffer[5]=crc_value;
          Radio.Send( TX_Buffer, 6);
    }
    else
    {
       Radio.Rx( RX_TIMEOUT_VALUE ); 
    }
    
    while( 1 )
    {
      
        Radio.IrqProcess( ); // Process Radio IRQ
        vTaskDelay(pdMS_TO_TICKS(1)); //延迟500ms
        // ESP_LOGI(TAG,"task......");
    }
#endif
    esp_err_t ret;
    ret = spi_driver_init();
    ESP_LOGI(TAG, "spi_driver[%d]", ret);
//    lora_app_create_task();
//    show_time();
//     while(1){
//         vTaskDelay(pdMS_TO_TICKS(50)); //延迟500ms
//         show_task();
       
//     }
    lorawan_main();
#endif
}




