#include "spi_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "esp_log.h"

static const char TAG[] = "spi_driver";

#define RESET_IO 3 // PB2
#define INTR_IO 4 //PA0
#define BUSY_IO 5 //PB10

#define CS_IO  10
#define CLK_IO 12
#define MOSI_IO 11
#define MISO_IO 13
static spi_device_handle_t spi;

//创建中断
static void _create_intr();

static void IRAM_ATTR spi_ready(spi_transaction_t *trans){



}


//增加cs
static void _add_cs_io(){

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<CS_IO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

}

esp_err_t spi_driver_init(){
    //初始化spi
    esp_err_t ret;
    ESP_LOGI(TAG, "Initializing bus SPI%d...", SPI2_HOST);
    spi_bus_config_t buscfg={
        .miso_io_num = MISO_IO,
        .mosi_io_num = MOSI_IO,
        .sclk_io_num = CLK_IO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024,
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=SPI_MASTER_FREQ_8M,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=-1,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=spi_ready,  //Specify pre-transfer callback to handle D/C line
    };
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(SPI2_HOST, &devcfg, &spi);


    
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<RESET_IO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<BUSY_IO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

#if 0
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<INTR_IO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
#else
    _create_intr();

#endif

    _add_cs_io();
    return ret;
}

//读取中断状态
int spi_driver_get_busy_io(){
   return gpio_get_level(BUSY_IO);
}

//读取中断状态
int spi_driver_get_intr_io(){
   return gpio_get_level(INTR_IO);
}

//设置重启状态
void spi_driver_set_reset(uint8_t val){
  gpio_set_level(RESET_IO, val);
}


//设置片选
void spi_driver_set_cs(uint8_t val){
  gpio_set_level(CS_IO, val);
}

//等待时间
void HAL_Delay_nMS( int val ){

  vTaskDelay(pdMS_TO_TICKS(val));
  return ;
}


esp_err_t SPI_SendData8(uint8_t data)
{
    esp_err_t err;
    err = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (err != ESP_OK) return err;

    spi_transaction_t t = {
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data = {data},
        .user = NULL,
    };
    err = spi_device_polling_transmit(spi, &t);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "transimt ok\n");
    }

    spi_device_release_bus(spi);
    return err;
}


/**
  * @brief  Returns the most recent received data by the SPIx/I2Sx peripheral. 
  * @param  SPIx: where x can be 1 or 2 in SPI mode to select the SPI peripheral. 
  * @note   SPI2 is not available for STM32F031 devices.
  * @retval The value of the received data.
  */
uint8_t SPI_ReceiveData8()
{
    spi_transaction_t t;


    memset(&t, 0, sizeof(t));
    t.length=8;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;
    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    assert( ret == ESP_OK );
    return t.rx_data[0];


}


/*!
 * @brief Sends txBuffer and receives rxBuffer
 *
 * @param [IN] txBuffer Byte to be sent
 * @param [OUT] rxBuffer Byte to be sent
 * @param [IN] size Byte to be sent
 */
uint8_t SpiInOut( uint8_t txBuffer)
{


    esp_err_t ret;
    spi_transaction_t t;


    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.flags = SPI_TRANS_USE_TXDATA|SPI_TRANS_USE_RXDATA;
    (t.tx_data)[0] = txBuffer;


    ret = spi_device_polling_transmit(spi, &t);
    assert( ret == ESP_OK );
    return t.rx_data[0]; 
   
}

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);

    //ESP_LOGI(TAG, "[%lu][%d]", gpio_num, gpio_get_level(gpio_num));
    
}


//创建中断
static void _create_intr(){
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_HIGH_LEVEL;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL<<INTR_IO);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    
    //下拉
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);



    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INTR_IO, gpio_isr_handler, (void*) INTR_IO);

}

uint8_t spi_driver_recv(){
    uint32_t io_num;
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
       
        return 1;
    }else{

        return 0;
    }
}
