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


