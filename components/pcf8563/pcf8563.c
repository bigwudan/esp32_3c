#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "pcf8563.h"

static const char *TAG = "PCF8563";

#define I2C_MASTER_NUM 0
#define I2C_MASTER_TIMEOUT_MS 1000

#define PCA8563_ADDR 0x51 /*!< Slave address of the PCA9535 */

// BCD转十进制
uint8_t bcd_decimal(uint8_t bcd)
{
    return bcd - (bcd >> 4) * 6;
}

//十进制转BCD
uint8_t decimal_bcd(uint8_t decimal)
{
    return (uint8_t)(decimal + (decimal / 10) * 6);
}

uint8_t time_s, time_m, time_h, time_D, time_M, time_Y;

void pcf8563_write_time(void)
{
    uint8_t writeBuff[8];
    writeBuff[0] = 0x02;            // s
    writeBuff[1] = decimal_bcd(0);  // s
    writeBuff[2] = decimal_bcd(59); // m
    writeBuff[3] = decimal_bcd(23); // h
    writeBuff[4] = decimal_bcd(27); // d
    writeBuff[5] = decimal_bcd(0);  // week
    writeBuff[6] = decimal_bcd(10); // M
    writeBuff[7] = decimal_bcd(22); //
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

void pcf8563_read_time(void)
{
#if 0
    uint8_t writeBuff[1], readBuff[1];

    writeBuff[0] = 0x02;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_s = bcd_decimal(readBuff[0] & 0x7f);

    writeBuff[0] = 0x03;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_m = bcd_decimal(readBuff[0] & 0x7f);

    writeBuff[0] = 0x04;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_h = bcd_decimal(readBuff[0] & 0x3f);

    writeBuff[0] = 0x05;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_D = bcd_decimal(readBuff[0] & 0x3f);

    writeBuff[0] = 0x07;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_M = bcd_decimal(readBuff[0] & 0x1f);

    writeBuff[0] = 0x08;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_Y = bcd_decimal(readBuff[0]);

    ESP_LOGI(TAG, "%d-%d-%d %d:%d:%d", time_Y, time_M, time_D, time_h, time_m, time_s);

#endif

    uint8_t writeBuff[1], readBuff[16];

    writeBuff[0] = 0x00;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA8563_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    time_s = bcd_decimal(readBuff[2] & 0x7f);
    time_m = bcd_decimal(readBuff[3] & 0x7f);
    time_h = bcd_decimal(readBuff[4] & 0x3f);
    time_D = bcd_decimal(readBuff[5] & 0x3f);
    time_M = bcd_decimal(readBuff[7] & 0x1f);
    time_Y = bcd_decimal(readBuff[8]);

    ESP_LOGI(TAG, "%d-%d-%d %d:%d:%d", time_Y, time_M, time_D, time_h, time_m, time_s);
}
