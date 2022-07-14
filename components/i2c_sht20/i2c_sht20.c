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
#define HOLD_AT_START 0xE3    //触发温度测量 0xE3
#define HOLD_AH_START 0xE3    //触发湿度测量
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

    int time_int = 0;
    i2c_get_timeout(i2c_master_port, &time_int);

    printf("xxxxxxxxxxxxxxxxx[%d]xx\n",time_int );

    i2c_set_timeout(i2c_master_port, 1000);

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


static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size){
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


    // vTaskDelay(70 / portTICK_RATE_MS);
    // cmd = i2c_cmd_link_create();
    // if (size > 1)
    // {
    //     i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    // }

    // i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    // i2c_master_stop(cmd);
    // ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    // i2c_cmd_link_delete(cmd);    
    return ret;

}

static esp_err_t i2c_master_read_slave_bk(i2c_port_t i2c_num, uint8_t *data_rd, size_t size)
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

    data_wr[0] = 0xF3;
    err = i2c_master_write_slave(I2C_MASTER_NUM, data_wr, strlen((char *)data_wr));
    printf("i2c_master_write_slave[%d]\n",err );
    vTaskDelay(70 / portTICK_RATE_MS);
    err = i2c_master_read_slave(I2C_MASTER_NUM, data_rx, 3);
    printf("i2c_master_read_slave[%d]\n",err );
    

    if(!data_rx[0]&&!data_rx[1]){
        return -1;
    }
    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = ((float)dat * 175.72) / 65536.0 - 46.85; // ℃




   return temp;
}



//得到温度
float i2c_sht20_get_humidity(){
    uint8_t data_rx[8] = {0};
    uint8_t data_wr[8] = {0};
    unsigned int dat = 0;
    float temp = 0;
    esp_err_t err;

    data_wr[0] = 0xF5;
    err = i2c_master_write_slave(I2C_MASTER_NUM, data_wr, strlen((char *)data_wr));
    printf("i2c_master_write_slave[%d]\n",err );
    vTaskDelay(70 / portTICK_RATE_MS);
    err = i2c_master_read_slave(I2C_MASTER_NUM, data_rx, 3);
    printf("i2c_master_read_slave[%d]\n",err );
    

    if(!data_rx[0]&&!data_rx[1]){
        return -1;
    }


    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = (float)((dat * 125.0) / 65536.0 - 6); //%RH
   return temp;
}

#if 1
void i2c_sht20_task(){
   float temp = 0;
   temp = i2c_sht20_get_temperature();

   printf("t=%.2f\n", temp);

    // temp = i2c_sht20_get_humidity();
    // printf("h=%.2f\n", temp);

   temp = i2c_sht20_get_humidity();
   printf("H=%.2f\n", temp);
   vTaskDelay(10000 / portTICK_RATE_MS);
   return ;
}
#else


static void _delay_i2c(){
    vTaskDelay(8 / portTICK_RATE_MS);
    vTaskDelay(5 / portTICK_RATE_MS);     
    return ;
}

#define I2C_SCL(A) do{gpio_set_level(I2C_MASTER_SCL_IO, A);}while(0)

#define I2C_SDA(A) do{gpio_set_level(I2C_MASTER_SDA_IO, A);}while(0)

//开始
static void _start_i2c(){
    I2C_SCL(1);
    _delay_i2c();
    I2C_SDA(1);
    _delay_i2c();
    I2C_SDA(0);
    return ;
}


static void _stop_i2c(){
    I2C_SCL(1);
    _delay_i2c();
    I2C_SDA(0);
    _delay_i2c();
    I2C_SDA(1);
    return ;
}


//写入地址
static void _write_i2c(uint8_t data){
    uint8_t flag = 0;
    for(int i=0; i<8; i++){
        flag = (0x01 & (data >> (7 -i)));
        //sda
        I2C_SCL(0);
        I2C_SDA(flag);
        _delay_i2c();
        I2C_SCL(1);  
        _delay_i2c();      
        I2C_SCL(0);  
    }
    return ;
}

//写入地址
static uint8_t _read_i2c(){
    uint8_t flag = 0;
    uint8_t res = 0;
 
    for(int i=0; i<8; i++){
        
        //sda
        I2C_SCL(0);
 
        _delay_i2c();
        I2C_SCL(1);  


        flag = gpio_get_level(I2C_MASTER_SDA_IO);

        if(flag){

           res = res | (1 << (7 - i));
        }

        _delay_i2c();      
        I2C_SCL(0);  
    }
    return res;
}


//等待ack
static uint8_t _get_ack(){
    uint8_t res = 0;
    //交出控制权
    I2C_SDA(1);

    I2C_SCL(0);
    _delay_i2c();
    I2C_SCL(1);
    res = gpio_get_level(I2C_MASTER_SDA_IO);
    _delay_i2c();      
    I2C_SCL(0);    
    return res;
}

//设置ack
static void _set_no_ack(uint8_t data){

        I2C_SCL(0);
        I2C_SDA(data);
        _delay_i2c();
        I2C_SCL(1);  
        _delay_i2c();      
        I2C_SCL(0); 
}

//发送写地址
static void _send_attr(){
    uint8_t flag_ack = 0;
    uint8_t data_rx[8] = {0};
    _start_i2c();
    //发送地址
    _write_i2c(0x80);
    flag_ack = _get_ack();

    if(flag_ack) return ;

    _write_i2c(0xE3);
    flag_ack = _get_ack();
    if(flag_ack) return ;

    _start_i2c();
    _write_i2c(0x81);
    flag_ack = _get_ack();

    if(flag_ack) return ;

    vTaskDelay(70 / portTICK_RATE_MS);

    data_rx[0] = _read_i2c();
    printf("data_1_[%d]\n",flag_ack );
    _set_no_ack(0);


     data_rx[1] = _read_i2c();
    printf("data_2_[%d]\n",flag_ack );    
    _set_no_ack(0);

    flag_ack = _read_i2c();
    printf("data_3_[%d]\n",flag_ack );
    _set_no_ack(1);    

    _stop_i2c();

    float temp;
        unsigned int dat = 0;
    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = ((float)dat * 175.72) / 65536.0 - 46.85; // ℃

    //printf("flag_ack[%d]\n",flag_ack );
printf("t=%.2f\n", temp);
    

}


//发送写地址
static void _send_attr_h(){
    uint8_t flag_ack = 0;
    uint8_t data_rx[8] = {0};
    _start_i2c();
    //发送地址
    _write_i2c(0x80);
    flag_ack = _get_ack();

    if(flag_ack) return ;

    _write_i2c(0xE5);
    flag_ack = _get_ack();
    if(flag_ack) return ;

    _start_i2c();
    _write_i2c(0x81);
    flag_ack = _get_ack();

    if(flag_ack) return ;

    vTaskDelay(70 / portTICK_RATE_MS);

    data_rx[0] = _read_i2c();
    printf("data_1_[%d]\n",flag_ack );
    _set_no_ack(0);


     data_rx[1] = _read_i2c();
    printf("data_2_[%d]\n",flag_ack );    
    _set_no_ack(0);

    flag_ack = _read_i2c();
    printf("data_3_[%d]\n",flag_ack );
    _set_no_ack(1);    

    _stop_i2c();

    float temp;
        unsigned int dat = 0;
    data_rx[1] &= 0xfc;
    dat = (data_rx[0] << 8) | data_rx[1];
    temp = (float)((dat * 125.0) / 65536.0 - 6); //%RH

    printf("H=%.2f\n", temp);
    

}



//发送写地址
static void _send_attr_bk(){
    uint8_t flag_ack = 0;
    _start_i2c();
    //发送地址
    _write_i2c(0x80);
    flag_ack = _get_ack();

    if(flag_ack) return ;

    _write_i2c(0xE7);
    flag_ack = _get_ack();
    if(flag_ack) return ;

    _start_i2c();
    _write_i2c(0x81);
    flag_ack = _get_ack();

    if(flag_ack) return ;
    flag_ack = _read_i2c();

    _stop_i2c();


    printf("flag_ack[%d]\n",flag_ack );

    

}


void i2c_sht20_task(){
    // gpio_pad_select_gpio(I2C_MASTER_SCL_IO);
	// gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    // gpio_pad_select_gpio(I2C_MASTER_SDA_IO);
	// gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    // _send_attr();

    // return ;
    gpio_pad_select_gpio(I2C_MASTER_SCL_IO);
	gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(I2C_MASTER_SCL_IO,GPIO_PULLUP_ONLY );

    gpio_pad_select_gpio(I2C_MASTER_SDA_IO);
	gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(I2C_MASTER_SDA_IO,GPIO_PULLUP_ONLY );

    I2C_SCL(1);  
    I2C_SDA(1);  
    printf("<<<<<<<<<<<<<<i2c_sht20_task\n");

    while(1){
        _send_attr_h();
        vTaskDelay(1000 / portTICK_RATE_MS);
        _send_attr();

    }




    return ;

    while(1){
        gpio_set_level(I2C_MASTER_SCL_IO, 0);
        gpio_set_level(I2C_MASTER_SDA_IO, 0);
        //vTaskDelay(5 / portTICK_RATE_MS);
        vTaskDelay(8 / portTICK_RATE_MS);
        vTaskDelay(5 / portTICK_RATE_MS);
        gpio_set_level(I2C_MASTER_SCL_IO, 1);
        gpio_set_level(I2C_MASTER_SDA_IO, 1);
        vTaskDelay(8 / portTICK_RATE_MS);
        vTaskDelay(5 / portTICK_RATE_MS);
    }



   return ;
}

#endif