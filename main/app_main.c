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

#include "radio.h"

#include "esp_log.h"
#include "driver/gpio.h"

/**************************************************************************************************************************************
Demo ³ÌÐòÁ÷³Ì  EnableMaster=true  ÎªÖ÷»ú¶Ë£¬Ö÷»ú¶Ë·¢ËÍÒ»¸ö"PING"Êý¾ÝºóÇÐ»»µ½½ÓÊÕ£¬µÈ´ý´Ó»ú·µ»ØµÄÓ¦´ð"PONG"Êý¾ÝLEDÉÁË¸

               EnableMaster=false Îª´Ó»ú¶Ë£¬´Ó»ú¶Ë½ÓÊÕµ½Ö÷»ú¶Ë·¢¹ýÀ´µÄ"PING"Êý¾ÝºóLEDÉÁË¸²¢·¢ËÍÒ»¸ö"PONG"Êý¾Ý×÷ÎªÓ¦´ð
***************************************************************************************************************************************/

#define USE_MODEM_LORA
//#define USE_MODEM_FSK

#define REGION_CN779

#if defined( REGION_AS923 )

#define RF_FREQUENCY                                923000000 // Hz

#elif defined( REGION_AU915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_CN779 )

#define RF_FREQUENCY                                433000000 // Hz

#elif defined( REGION_EU868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( REGION_KR920 )

#define RF_FREQUENCY                                920000000 // Hz

#elif defined( REGION_IN865 )

#define RF_FREQUENCY                                865000000 // Hz

#elif defined( REGION_US915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_US915_HYBRID )

#define RF_FREQUENCY                                915000000 // Hz

#else

    #error "Please define a frequency band in the compiler options."

#endif

#define TX_OUTPUT_POWER                             22        // 22 dBm

extern bool IrqFired;




bool EnableMaster=true;//Ö÷´ÓÑ¡Ôñ

uint16_t  crc_value;
/*!
 * Radio events function pointer
 */

static RadioEvents_t RadioEvents;

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,    
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       9         // [SF7..SF12]    
#define LORA_CODINGRATE                             4         // [1: 4/5,       
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    5e3      // Hz 
#define FSK_DATARATE                                2.4e3      // bps
#define FSK_BANDWIDTH                               20e3     // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH                           100e3     // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t TX_Buffer[BUFFER_SIZE];
uint8_t RX_Buffer[BUFFER_SIZE];


States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;
void OnTxDone( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( void );
void OnRxTimeout( void );
void OnRxError( void );
static void _radio_init();


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
    _radio_init();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

    }    
#endif
}


void OnTxDone( void )
{   
    Radio.Standby();
    Radio.Rx( RX_TIMEOUT_VALUE ); //½øÈë½ÓÊÕ

}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    // BufferSize = size;
    // memcpy( RX_Buffer, payload, BufferSize );
    // RssiValue = rssi;
    // SnrValue = snr;
    
    // Radio.Standby();
    
    // if(EnableMaster)
    // {
    //   if(memcmp(RX_Buffer,PongMsg,4)==0)
    //   {
    //     LedToggle();//LEDÉÁË¸
        
    //   }
     
    //     TX_Buffer[0] = 'P';
    //     TX_Buffer[1] = 'I';
    //     TX_Buffer[2] = 'N';
    //     TX_Buffer[3] = 'G'; 
        
    //     crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
    //     TX_Buffer[4]=crc_value>>8;
    //     TX_Buffer[5]=crc_value;
    //     Radio.Send( TX_Buffer, 6);
    // }
    // else
    // {
    //   if(memcmp(RX_Buffer,PingMsg,4)==0)
    //   {
    //     LedToggle();//LEDÉÁË¸
        
    //     TX_Buffer[0] = 'P';
    //     TX_Buffer[1] = 'O';
    //     TX_Buffer[2] = 'N';
    //     TX_Buffer[3] = 'G'; 
        
    //     crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
    //     TX_Buffer[4]=crc_value>>8;
    //     TX_Buffer[5]=crc_value;
    //     Radio.Send( TX_Buffer, 6);
    //   }
    //   else
    //   {
    //     Radio.Rx( RX_TIMEOUT_VALUE ); 
    //   }   
    // }
}

void OnTxTimeout( void )
{
   
}

void OnRxTimeout( void )
{
    // Radio.Standby();
    // if(EnableMaster)
    // {
    //     TX_Buffer[0] = 'P';
    //     TX_Buffer[1] = 'I';
    //     TX_Buffer[2] = 'N';
    //     TX_Buffer[3] = 'G'; 
        
    //     crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
    //     TX_Buffer[4]=crc_value>>8;
    //     TX_Buffer[5]=crc_value;
    //     Radio.Send( TX_Buffer, 6);
    // }
    // else
    // {
    //   Radio.Rx( RX_TIMEOUT_VALUE ); 
    // }
}

void OnRxError( void )
{

    // Radio.Standby();
    // if(EnableMaster)
    // {
    //     TX_Buffer[0] = 'P';
    //     TX_Buffer[1] = 'I';
    //     TX_Buffer[2] = 'N';
    //     TX_Buffer[3] = 'G'; 
        
    //     crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
    //     TX_Buffer[4]=crc_value>>8;
    //     TX_Buffer[5]=crc_value;
    //     Radio.Send( TX_Buffer, 6);
    // }
    // else
    // {
    //   Radio.Rx( RX_TIMEOUT_VALUE ); 
    // }
  
}

static void _radio_init(){
    // Radio initialization
    // RadioEvents.TxDone = OnTxDone;
    // RadioEvents.RxDone = OnRxDone;
    // RadioEvents.TxTimeout = OnTxTimeout;
    // RadioEvents.RxTimeout = OnRxTimeout;
    // RadioEvents.RxError = OnRxError;
    
    
    Radio.Init( &RadioEvents );


    //Radio.SetChannel( RF_FREQUENCY );

    uint8_t data[2]={0x12,0x34};
    static uint8_t test[2] = {0};
    //Radio.WriteBuffer(0x06C0,data,2);
    while(1){


        
        Radio.WriteBuffer(0x06C0,data,2);
        Radio.ReadBuffer(0x06C0,test,2);


        ESP_LOGI("read:", "[%02X][%02X]", test[0], test[1]);

        vTaskDelay(pdMS_TO_TICKS(500)); //延迟500ms
    }



}