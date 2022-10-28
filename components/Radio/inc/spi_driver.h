#ifndef _SPI_DRIVER_H_
#define _SPI_DRIVER_H_

#include <stdint.h>
#include <esp_err.h>

#include "pca9535.h"

esp_err_t spi_driver_init();


//读取中断状态
int spi_driver_get_busy_io();
//读取中断状态
int spi_driver_get_intr_io();
//设置重启状态
void spi_driver_set_reset(uint8_t val);

//设置片选
void spi_driver_set_cs(uint8_t val);

//等待时间
void HAL_Delay_nMS( int val );


/*!
 * @brief Sends txBuffer and receives rxBuffer
 *
 * @param [IN] txBuffer Byte to be sent
 * @param [OUT] rxBuffer Byte to be sent
 * @param [IN] size Byte to be sent
 */
uint8_t SpiInOut( uint8_t txBuffer);

esp_err_t SPI_SendData8(uint8_t data);

uint8_t SPI_ReceiveData8();

uint8_t spi_driver_recv();
#endif

