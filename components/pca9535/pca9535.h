#include <stdio.h>

#ifndef __PCA9535_H

#define __PCA9535_H

typedef union
{
    struct
    {
        uint8_t bit0 : 1;
        uint8_t bit1 : 1;
        uint8_t bit2 : 1;
        uint8_t bit3 : 1;
        uint8_t bit4 : 1;
        uint8_t bit5 : 1;
        uint8_t bit6 : 1;
        uint8_t bit7 : 1;
    } bits;

    uint8_t byte;
} ExtIO_Reg_Type;

extern ExtIO_Reg_Type extIO_OutReg[2]; // 2组io
extern ExtIO_Reg_Type extIO_InReg[2];  // 2组io

void pca9535_read_input(void);
void pca9535_write_outreg(uint8_t port0_Reg, uint8_t port1_Reg);
void pca9535_write_outpin(uint8_t portNum, uint8_t pinNum, uint8_t value);
void pca9535_init(void);

/**
 * @brief 读取当个IO状态
 * 
 * @param portNum 端口号
 * @param pinNum  pin号
 * @return uint8_t 
 */
uint8_t pca9535_read_inpin(uint8_t portNum, uint8_t pinNum);
#endif