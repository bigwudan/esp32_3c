#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "pca9535.h"

static const char *TAG = "PCA9535";

#define I2C_MASTER_NUM 0
#define I2C_MASTER_TIMEOUT_MS 1000

#define PCA9535_ADDR 0x20 /*!< Slave address of the PCA9535 */

ExtIO_Reg_Type extIO_OutReg[2] = {0x00}; // 2组io
ExtIO_Reg_Type extIO_InReg[2] = {0x00};  // 2组io




//static const char *TAG = "i2c-simple-example";

#define I2C_MASTER_SCL_IO 2         /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO 1         /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM 0            /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ 100000   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

/**
 * @brief i2c master initialization
 */
static esp_err_t
i2c_master_init(void)
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

static void extio_outreg_write_bit(uint8_t regNum, uint8_t bitNum, uint8_t value)
{
    if (value)
    {
        extIO_OutReg[regNum].byte = extIO_OutReg[regNum].byte | (0x01 << bitNum);
    }
    else
    {
        extIO_OutReg[regNum].byte = extIO_OutReg[regNum].byte & ~(0x01 << bitNum);
    }
}

void pca9535_init(void)
{
    //输入输出设置
    // IO_0 = 0001 1111  0x1F
    // IO_1 = 0110 0000  0x00
    uint8_t pca9535_setbuff[3] = {0x06, 0x1F, 0x60};

    i2c_master_init();
    
    ESP_LOGI(TAG, "设置输入输出");



    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, PCA9535_ADDR, pca9535_setbuff, sizeof(pca9535_setbuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));


    extIO_OutReg[0].byte = 0xE0;
    extIO_OutReg[1].byte = 0x19;

    pca9535_setbuff[0] = 0x02;
    pca9535_setbuff[1] = extIO_OutReg[0].byte;
    pca9535_setbuff[2] = extIO_OutReg[1].byte;
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, PCA9535_ADDR, pca9535_setbuff, sizeof(pca9535_setbuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));

}

void pca9535_write_outpin(uint8_t portNum, uint8_t pinNum, uint8_t value)
{
    uint8_t writeBuff[3] = {0x00};

    extio_outreg_write_bit(portNum, pinNum, value);
    writeBuff[0] = 0x02; // output port reg
    writeBuff[1] = extIO_OutReg[0].byte;
    writeBuff[2] = extIO_OutReg[1].byte;

    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, PCA9535_ADDR, writeBuff, sizeof(writeBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

void pca9535_write_outreg(uint8_t port0_Reg, uint8_t port1_Reg)
{
    uint8_t writeBuff[3] = {0x02, port0_Reg, port1_Reg};
    extIO_OutReg[0].byte = port0_Reg;
    extIO_OutReg[1].byte = port1_Reg;

    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, PCA9535_ADDR, writeBuff, sizeof(writeBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}

void pca9535_read_input(void)
{
    const uint8_t writeBuff[] = {0x00};
    uint8_t readBuff[2];
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, PCA9535_ADDR, writeBuff, sizeof(writeBuff), readBuff, sizeof(readBuff), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    extIO_InReg[0].byte = readBuff[0];
    extIO_InReg[1].byte = readBuff[1];

#if 0
    ESP_LOGI(TAG, "输入寄存器 REG0 = 0x%02X, REG1 = 0x%02X", readBuff[0], readBuff[1]);
    ESP_LOGI(TAG, "\r\n REG0_0 = %d\r\n REG0_1 = %d\r\n REG0_2 = %d\r\n REG0_3 = %d\r\n REG0_4 = %d\r\n REG0_5 = %d\r\n REG0_6 = %d\r\n REG0_7 = %d\r\n",
             extIO_InReg[0].bits.bit0,
             extIO_InReg[0].bits.bit1,
             extIO_InReg[0].bits.bit2,
             extIO_InReg[0].bits.bit3,
             extIO_InReg[0].bits.bit4,
             extIO_InReg[0].bits.bit5,
             extIO_InReg[0].bits.bit6,
             extIO_InReg[0].bits.bit7);
    ESP_LOGI(TAG, "\r\n REG1_0 = %d\r\n REG1_1 = %d\r\n REG1_2 = %d\r\n REG1_3 = %d\r\n REG1_4 = %d\r\n REG1_5 = %d\r\n REG1_6 = %d\r\n REG1_7 = %d\r\n",
             extIO_InReg[1].bits.bit0,
             extIO_InReg[1].bits.bit1,
             extIO_InReg[1].bits.bit2,
             extIO_InReg[1].bits.bit3,
             extIO_InReg[1].bits.bit4,
             extIO_InReg[1].bits.bit5,
             extIO_InReg[1].bits.bit6,
             extIO_InReg[1].bits.bit7);
#endif             
}

/**
 * @brief 读取当个IO状态
 * 
 * @param portNum 端口号
 * @param pinNum  pin号
 * @return uint8_t 
 */
uint8_t pca9535_read_inpin(uint8_t portNum, uint8_t pinNum){
    uint8_t res = 0;
    pca9535_read_input();
    switch (pinNum)
    {
    case 0:
        res = extIO_InReg[portNum].bits.bit0;
        break;
    case 1:
        res = extIO_InReg[portNum].bits.bit1;
        break;
    case 2:
        res = extIO_InReg[portNum].bits.bit2;
        break;
    case 3:
        res = extIO_InReg[portNum].bits.bit3;
        break;
    case 4:
        res = extIO_InReg[portNum].bits.bit4;
        break;
    case 5:
        res = extIO_InReg[portNum].bits.bit5;
        break;
    case 6:
        res = extIO_InReg[portNum].bits.bit6;
        break;
    case 7:
        res = extIO_InReg[portNum].bits.bit7;
        break;
    default:
        break;
    }
    return res;
}