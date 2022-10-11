#include "spi_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "esp_log.h"

static const char TAG[] = "spi_driver";

#define RESET_IO 1
#define INTR_IO 2
#define BUSY_IO 3

#define CS_IO  2
#define CLK_IO 3
#define MOSI_IO 3
#define MISO_IO 4
static spi_device_handle_t spi;

static void IRAM_ATTR spi_ready(spi_transaction_t *trans){



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
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=SPI_MASTER_FREQ_40M,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=CS_IO,               //CS pin
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


    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<INTR_IO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

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

//等待时间
void HAL_Delay_nMS( int val ){

  vTaskDelay(pdMS_TO_TICKS(10));
  return ;
}


void SPI_SendData8(uint8_t Data)
{
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 8;
	t.tx_buffer = &Data;
	t.user = (void*)0;

	ret = spi_device_polling_transmit(spi, &t);
	if(ret != ESP_OK){
		printf("*********lcd_cmd err************\n");
	}
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
#if 0    
      while( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);//µ±·¢ËÍbufferÎª¿ÕÊ±(ËµÃ÷ÉÏÒ»´ÎÊý¾ÝÒÑ¸´ÖÆµ½ÒÆÎ»¼Ä´æÆ÷ÖÐ)ÍË³ö,ÕâÊ±¿ÉÒÔÍùbufferÀïÃæÐ´Êý¾Ý
      SPI_SendData8(SPI2, txBuffer);
    
      while( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);//µ±½ÓÊÕbufferÎª·Ç¿ÕÊ±ÍË³ö
      return SPI_ReceiveData8(SPI2);
 #else
    SPI_SendData8(txBuffer);
    return SPI_ReceiveData8();
 #endif     
   
}