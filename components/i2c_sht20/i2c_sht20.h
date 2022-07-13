#ifndef _I2C_SH20_H
#define _I2C_SH20_H

#include <stdio.h>

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void);

void i2c_sht20_task();

#endif