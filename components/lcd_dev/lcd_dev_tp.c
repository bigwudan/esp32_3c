/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "lcd_dev.h"


//tp i2c
#define I2C_MASTER_INT_IO           17
#define I2C_MASTER_SCL_IO           8      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           18      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          50000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define ESP_SLAVE_ADDR 0x38

#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */

#define WRITE_BIT 0 /*!< I2C master write */
#define READ_BIT 1   /*!< I2C master read */

#define FT_DEVIDE_MODE 			0x00   		
#define FT_REG_NUM_FINGER       0x02		
#define FT_TP1_REG 				0X03	  	
#define FT_TP2_REG 				0X09		
#define FT_TP3_REG 				0X0F		
#define FT_TP4_REG 				0X15		
#define FT_TP5_REG 				0X1B		
#define	FT_ID_G_LIB_VERSION		0xA1		
#define FT_ID_G_MODE 			0xA4   		
#define FT_ID_G_THGROUP			0x80   		
#define FT_ID_G_PERIODACTIVE	0x88  




static tp_data_tag tp_data;

/**
 * @brief Write a byte to a MPU9250 sensor register
 */
static esp_err_t i2c_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, ESP_SLAVE_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret;
}

/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
static esp_err_t i2c_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, ESP_SLAVE_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static void _tp_5206_init(){
    uint8_t data_rx[8] = {0};


    esp_err_t err;
  

    ESP_ERROR_CHECK(i2c_register_write_byte(FT_DEVIDE_MODE, 0));

    //FT_ID_G_MODE
    ESP_ERROR_CHECK(i2c_register_write_byte(FT_ID_G_MODE, 0));

    ESP_ERROR_CHECK(i2c_register_write_byte(FT_ID_G_THGROUP,28));

    ESP_ERROR_CHECK(i2c_register_write_byte(FT_ID_G_PERIODACTIVE,12));

    
    
    ESP_ERROR_CHECK(i2c_register_read(FT_ID_G_LIB_VERSION, data_rx, 2));
    ESP_ERROR_CHECK(i2c_register_read(FT_ID_G_THGROUP, data_rx, 1));
    ESP_ERROR_CHECK(i2c_register_read(FT_ID_G_PERIODACTIVE, data_rx, 1));

   
}

/**
 * @brief i2c master initialization
 */
static esp_err_t _i2c_master_init(void)
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


//扫描
void lcd_dev_tp_scan_tp(){
    uint16_t x_pos = 0;
    uint16_t y_pos = 0;
    uint8_t read_buf[8] = {0};
    i2c_register_read(FT_REG_NUM_FINGER, read_buf, 1);
    if(gpio_get_level(I2C_MASTER_INT_IO) == 0){
        if(read_buf[0] == 1){
            i2c_register_read(FT_TP1_REG, read_buf, 4);
            if((read_buf[0]&0X80)!=0X80) return;
            x_pos=((uint16_t)(read_buf[0]&0X0F)<<8)+read_buf[1];
            y_pos=((uint16_t)(read_buf[2]&0X0F)<<8)+read_buf[3];
            
            tp_data.x_pos = x_pos;
            tp_data.y_pos = y_pos;
            tp_data.press_state = 1;
        }else{
            tp_data.x_pos = 0;
            tp_data.y_pos = 0;
            tp_data.press_state = 0;

        }
    }else{
        tp_data.x_pos = 0;
        tp_data.y_pos = 0;
        tp_data.press_state = 0;

    }



    return ;
}



void lcd_dev_tp_init(){
    _i2c_master_init();
    _tp_5206_init();


        //zero-initialize the config structure.
    gpio_config_t io_conf = {};
	//interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = 1ULL<<I2C_MASTER_INT_IO;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    return ;
}

tp_data_tag *lcd_dev_tp_get_data(){

    return &tp_data;
}