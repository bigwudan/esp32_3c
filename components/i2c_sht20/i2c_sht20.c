/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"



#define I2C_MASTER_SCL_IO           8      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           2      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          50000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000


#define ESP_SLAVE_ADDR 0x40

#define SHT_SENSOR_ADDR                 0x80        /*传感器地址0x80 */
#define HOLD_AT_START 0xF5    //触发温度测量 0xE3
#define HOLD_AH_START 0xF5    //触发湿度测量
#define REST 0xfe             //软件复位

#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */

#define WRITE_BIT 0 /*!< I2C master write */
#define READ_BIT 1   /*!< I2C master read */

//i2c
#include "driver/i2c.h"

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


/**
 * @brief Write a byte to a MPU9250 sensor register
 */
static esp_err_t _register_write_byte(uint8_t* data, uint8_t len)
{
    int ret;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SHT_SENSOR_ADDR, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    return ret;
}


/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
static esp_err_t _register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, SHT_SENSOR_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}


static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT , ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size)
{
    if (size == 0)
    {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);

    if (size > 1)
    {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }

    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

//得到温度
float i2c_sht20_get_temperature(){
    uint8_t data_rx[8] = {0};
    uint8_t data_wr[8] = {0};
    unsigned int dat = 0;
    float temp = 0;
    esp_err_t err;

    data_wr[0] = HOLD_AH_START;
    err = i2c_master_write_slave(I2C_MASTER_NUM, data_wr, strlen((char *)data_wr));
    printf("i2c_master_write_slave[%d]\n",err );
    vTaskDelay(30 / portTICK_RATE_MS);
    err = i2c_master_read_slave(I2C_MASTER_NUM, data_rx, 3);
    printf("i2c_master_read_slave[%d]\n",err );
   printf("read[%02X][%02X][%02X]\n",data_rx[0], data_rx[1],data_rx[2] );

    if(!data_rx[0]&&!data_rx[1]){
        return -1;
    }
/*    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = ((float)dat * 175.72) / 65536.0 - 46.85; // ℃
*/


    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = (float)((dat * 125.0) / 65536.0 - 6); //%RH
   return temp;
}



//得到温度
float i2c_sht20_get_humidity(){
     uint8_t data_rx[8] = {0};
    uint8_t data_wr[8] = {0};
    unsigned int dat = 0;
    float temp = 0;
    esp_err_t err;

    data_wr[0] = HOLD_AH_START;
    err = i2c_master_write_slave(I2C_MASTER_NUM, data_wr, strlen((char *)data_wr));
    printf("i2c_master_write_slave[%d]\n",err );
    vTaskDelay(30 / portTICK_RATE_MS);
    err = i2c_master_read_slave(I2C_MASTER_NUM, data_rx, 3);
    printf("i2c_master_read_slave[%d]\n",err );
   printf("read[%02X][%02X][%02X]\n",data_rx[0], data_rx[1],data_rx[2] );

    if(!data_rx[0]&&!data_rx[1]){
        return -1;
    }
/*    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = ((float)dat * 175.72) / 65536.0 - 46.85; // ℃
*/


    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = (float)((dat * 125.0) / 65536.0 - 6); //%RH
   return temp;
}


void i2c_sht20_task(){
   float temp = 0;
   temp = i2c_sht20_get_humidity();

   printf("H=%.2f\n", temp);

//    temp = i2c_sht20_get_humidity();
//    printf("H=%.2f\n", temp);
   //vTaskDelay(10000 / portTICK_RATE_MS);
   return ;
}
