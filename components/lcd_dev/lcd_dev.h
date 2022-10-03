#ifndef _LCD_DEV_H
#define _LCD_DEV_H

#include <stdio.h>
typedef struct{
    uint16_t x_pos;
    uint16_t y_pos;
    uint8_t press_state; // 0放开 1按下


}tp_data_tag;

void lcd_dev_init();

void lcd_dev_task();

void lcd_dev_tp_init();



//扫描
void lcd_dev_tp_scan_tp();

tp_data_tag *lcd_dev_tp_get_data();

void lcd_dev_set_bklight(uint8_t var_num);
#endif