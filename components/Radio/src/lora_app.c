#include "spi_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "esp_log.h"

#include "lora_app.h"

#include "radio.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "crc.h"


static const char TAG[] = "lora_app";

//static const char TAG[] = "lora_app";

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

#define LedToggle() do{ESP_LOGI(TAG, "[%s][%d]", __func__, __LINE__);}while(0)

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t TX_Buffer[BUFFER_SIZE];
uint8_t RX_Buffer[BUFFER_SIZE];


States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;
static void OnTxDone( void );
static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnTxTimeout( void );
static void OnRxTimeout( void );
static void OnRxError( void );

//创建任务
static void _create_task();


static void OnTxDone( void )
{  
    ESP_LOGI("xx", "[%s][%d]", __func__, __LINE__);     
    Radio.Standby();
    Radio.Rx( RX_TIMEOUT_VALUE ); //½øÈë½ÓÊÕ

}

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    BufferSize = size;
    memcpy( RX_Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    ESP_LOGI("xx", "[%s][%d]", __func__, __LINE__);  
    Radio.Standby();
    
    printf("lora_recv:");
    for(int i=0; i<size; i++ ){
        printf("[%02X]", payload[i]);

    }
    printf("\n");

    TX_Buffer[0] = 'P';
    TX_Buffer[1] = 'I';
    TX_Buffer[2] = 'N';
    TX_Buffer[3] = 'G'; 
    
    crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//¼ÆËãµÃ³öÒª·¢ËÍÊý¾Ý°üCRCÖµ
    TX_Buffer[4]=crc_value>>8;
    TX_Buffer[5]=crc_value;
    Radio.Send( TX_Buffer, 6);


  
}

static void OnTxTimeout( void )
{
   ESP_LOGI("xx", "[%s][%d]", __func__, __LINE__);
}

static void OnRxTimeout( void )
{
    ESP_LOGI("xx", "[%s][%d]", __func__, __LINE__); 
}

static void OnRxError( void )
{
    //printf("[%s][%d]\n", );
    ESP_LOGI("xx", "[%s][%d]", __func__, __LINE__);
  
}

static void lora_task_worker(void *aContext){



    //Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );

#if 0
   uint8_t data[2]={0x12,0x34};
   static uint8_t test[2];
   Radio.WriteBuffer(0x06C0,data,2);
   Radio.ReadBuffer(0x06C0,test,2);
   ESP_LOGI(TAG, "test[%02X][%02X]", test[0], test[1]);
#endif

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
#if 0        
        if(spi_driver_recv() == 1){
            Radio.IrqProcess( ); // Process Radio IRQ
        }
#else
        Radio.IrqProcess( ); // Process Radio IRQ
        vTaskDelay(pdMS_TO_TICKS(5)); //延迟500ms
        
#endif        

    }

    return ;
}

//创建任务
void lora_app_create_task(){
    ESP_LOGI(TAG, "lora_create");
    xTaskCreate(lora_task_worker, "ot_br_main", 4096, xTaskGetCurrentTaskHandle(), 1, NULL);

}



void lora_app_init(){


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
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    
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


}


void lora_app_set_pingpong(){
    Radio.Rx( RX_TIMEOUT_VALUE ); 
}

void lora_app_process(){
    Radio.IrqProcess( ); // Process Radio IRQ

}