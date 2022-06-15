#ifndef _LCD_DEV_H
#define _LCD_DEV_H

#include <stdio.h>


#define PINK 0xD2F5
#define BLUE 0X03bD
#define RED 0xF882
#define YELLOW 0xFFE0
#define WHITE 0xffff
#define BLACK 0x0000

#define X_MAX_PIXEL 240
#define Y_MAX_PIXEL 240

#define SPI_SPEED_RAM 20
#define SPI_SPEED X_MAX_PIXEL*(SPI_SPEED_RAM)
#define SPI_SPEED_BUF_NUM SPI_SPEED*2

uint32_t lcd_dev_init();

void Lcd_Fill( uint16_t x_start, uint16_t y_start,
		uint16_t x_end, uint16_t y_end, uint16_t Color);


void Lcd_ClearPort( uint16_t Color);


void Lcd_ClearPort_test( uint16_t Color);


void Lcd_Fill_num(uint16_t x_start, uint16_t y_start,
	uint16_t x_end, uint16_t y_end, uint32_t Color_len,uint8_t *Color_buf);
#endif