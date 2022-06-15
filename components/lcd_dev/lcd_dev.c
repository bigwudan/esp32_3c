/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//spi lcd
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "lcd_dev.h"
#define USE_HORIZONTAL 1

#define TFT_WRITE_COMMAND(A) lcd_cmd(A)
#define TFT_WRITE_DATA(A) 	lcd_data(A)





#define SDA 3
#define SCK 2
#define CS 7
#define IO_CD 6
#define IO_BL 11
#define IO_RESET 10


#define delay_lcd(A) do{vTaskDelay((A) / portTICK_PERIOD_MS);}while(0)




 typedef struct
 {
 	uint16_t width;
 	uint16_t height;
 	uint16_t id;
 	uint8_t  dir;
 	uint16_t	 wramcmd;
 	uint16_t  setxcmd;
 	uint16_t setycmd;
 	uint8_t   xoffset;
 	uint8_t	 yoffset;
 }_lcd_dev;

_lcd_dev  lcddev;
static spi_device_handle_t spi; 

static void IRAM_ATTR spi_ready(spi_transaction_t *trans){



}


static const spi_bus_config_t buscfg = {
.miso_io_num = -1,
.mosi_io_num = SDA,
.sclk_io_num = SCK,
.quadhd_io_num = -1,
.quadwp_io_num = -1,
.max_transfer_sz = SPI_SPEED_BUF_NUM
};

static const spi_device_interface_config_t devcfg = {
.clock_speed_hz = SPI_MASTER_FREQ_80M,
.mode = 0,
.spics_io_num =CS,
.queue_size = 7,
.cs_ena_pretrans = 10,
.post_cb = spi_ready


};


//IO口初始化
static int _vspi_init(){
	esp_err_t err;


	//command/data
	gpio_pad_select_gpio(IO_CD);
	gpio_set_direction(IO_CD, GPIO_MODE_OUTPUT);


	
	gpio_pad_select_gpio(IO_BL);
	gpio_set_direction(IO_BL, GPIO_MODE_OUTPUT);
	gpio_set_level(IO_BL, 1);

	
	gpio_pad_select_gpio(IO_RESET);
	gpio_set_direction(IO_RESET, GPIO_MODE_OUTPUT);
	gpio_set_level(IO_RESET, 1);

	err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

	if(err != ESP_OK){
		return -1;

	}

	err = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

	if(err != ESP_OK){
		return -1;

	}
	return 0;
}


//
int lcd_cmd(const uint8_t cmd){
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 8;
	t.tx_buffer = &cmd;
	t.user = (void*)0;
	gpio_set_level(IO_CD, 0);
	ret = spi_device_polling_transmit(spi, &t);
	if(ret != ESP_OK){
		printf("*********lcd_cmd err************\n");


	}
	//assert(ret == ESP_OK);
	return ret;
}

//
int lcd_data(const uint8_t cmd){
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 8;
	t.tx_buffer = &cmd;
	t.user = (void*)0;
	gpio_set_level(IO_CD, 1);
	ret = spi_device_polling_transmit(spi, &t);
	if(ret != ESP_OK){
		printf("*********lcd_cmd err************\n");


	}
	//assert(ret == ESP_OK);
	return ret;
}


//
int lcd_16_data(const uint8_t *cmd){
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 16;
	t.tx_buffer = cmd;
	t.user = (void*)0;
	gpio_set_level(IO_CD, 1);
	ret = spi_device_polling_transmit(spi, &t);
	if(ret != ESP_OK){
		printf("*********lcd_cmd err************\n");


	}
	//assert(ret == ESP_OK);
	return ret;


}

int lcd_spi_data_by_num(const uint8_t *cmd, uint32_t len){
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = len*8;
	t.tx_buffer = cmd;
	t.user = (void*)0;
	gpio_set_level(IO_CD, 1);



	ret = spi_device_polling_transmit(spi, &t);
	if(ret != ESP_OK){
		printf("*********lcd_cmd err************\n");


	}
	//assert(ret == ESP_OK);
	return ret;


}



static void _096lcd(){


	gpio_set_level(IO_RESET, 1);
    //vTaskDelay(200 / portTICK_PERIOD_MS);
    delay_lcd(200);

	gpio_set_level(IO_RESET, 0);
    delay_lcd(800);

    gpio_set_level(IO_RESET, 1);
    delay_lcd(800);

delay_lcd(120);                //ms

TFT_WRITE_COMMAND(0x11);     //Sleep out

delay_lcd(120);                //Delay 120ms

TFT_WRITE_COMMAND(0x36);     
TFT_WRITE_DATA(0x00);   

TFT_WRITE_COMMAND(0x21);     

TFT_WRITE_COMMAND(0xB2);     
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x00);   
TFT_WRITE_DATA(0x33);   
TFT_WRITE_DATA(0x33);   

TFT_WRITE_COMMAND(0xB7);     
TFT_WRITE_DATA(0x75);   

TFT_WRITE_COMMAND(0xBB);     
TFT_WRITE_DATA(0x22);   

TFT_WRITE_COMMAND(0xC0);     
TFT_WRITE_DATA(0x2C);   

TFT_WRITE_COMMAND(0xC2);     
TFT_WRITE_DATA(0x01);   

TFT_WRITE_COMMAND(0xC3);     
TFT_WRITE_DATA(0x13);   

TFT_WRITE_COMMAND(0xC4);     
TFT_WRITE_DATA(0x20);   

TFT_WRITE_COMMAND(0xC6);     
TFT_WRITE_DATA(0x11);   

TFT_WRITE_COMMAND(0xD0);     
TFT_WRITE_DATA(0xA4);   
TFT_WRITE_DATA(0xA1);   

TFT_WRITE_COMMAND(0xD6);     
TFT_WRITE_DATA(0xA1);   

TFT_WRITE_COMMAND(0xE0);     
TFT_WRITE_DATA(0xD0);   
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x0A);   
TFT_WRITE_DATA(0x09);   
TFT_WRITE_DATA(0x08);   
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x2E);   
TFT_WRITE_DATA(0x44);   
TFT_WRITE_DATA(0x45);   
TFT_WRITE_DATA(0x0F);   
TFT_WRITE_DATA(0x17);   
TFT_WRITE_DATA(0x16);   
TFT_WRITE_DATA(0x2B);   
TFT_WRITE_DATA(0x33);   

TFT_WRITE_COMMAND(0xE1);     
TFT_WRITE_DATA(0xD0);   
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x0A);   
TFT_WRITE_DATA(0x09);   
TFT_WRITE_DATA(0x08);   
TFT_WRITE_DATA(0x05);   
TFT_WRITE_DATA(0x2E);   
TFT_WRITE_DATA(0x43);   
TFT_WRITE_DATA(0x45);   
TFT_WRITE_DATA(0x0F);   
TFT_WRITE_DATA(0x16);   
TFT_WRITE_DATA(0x16);   
TFT_WRITE_DATA(0x2B);   
TFT_WRITE_DATA(0x33);   

TFT_WRITE_COMMAND(0x3A);     
TFT_WRITE_DATA(0x55);

TFT_WRITE_COMMAND(0x29);     //Display on
TFT_WRITE_COMMAND(0x2C);            //Delay 120ms

}


static void _096lcd_bk(){
	TFT_WRITE_COMMAND(0x11);//Sleep exit
	vTaskDelay(120 / portTICK_PERIOD_MS);

	TFT_WRITE_COMMAND(0xB1); //֡��
	TFT_WRITE_DATA(0x05);
	TFT_WRITE_DATA( 0x3A);
	TFT_WRITE_DATA( 0x3A);

	TFT_WRITE_COMMAND(0xB2); //֡��
	TFT_WRITE_DATA(0x05);
	TFT_WRITE_DATA(0x3A);
	TFT_WRITE_DATA(0x3A);

     TFT_WRITE_COMMAND(0xB3); //֡��
     TFT_WRITE_DATA(0x05);
     TFT_WRITE_DATA(0x3A);
     TFT_WRITE_DATA(0x3A);
     TFT_WRITE_DATA(0x05);
     TFT_WRITE_DATA( 0x3A);
     TFT_WRITE_DATA( 0x3A);

     TFT_WRITE_COMMAND(0x21); //������(�õ��ǵ��ԵĻ����Լ�ת����RGB565���Կ�����)
     TFT_WRITE_COMMAND( 0xB4); //����control
     TFT_WRITE_DATA( 0x03);

     TFT_WRITE_COMMAND( 0xC0); //���� control
     TFT_WRITE_DATA(0x62);
     TFT_WRITE_DATA( 0x02);
     TFT_WRITE_DATA( 0x04);

     TFT_WRITE_COMMAND(0xC1);
     TFT_WRITE_DATA( 0xC0);

     TFT_WRITE_COMMAND( 0xC2);
     TFT_WRITE_DATA( 0x0D);
     TFT_WRITE_DATA( 0x00);

     TFT_WRITE_COMMAND(0xC3);
     TFT_WRITE_DATA( 0x8D);
     TFT_WRITE_DATA( 0x6A);

     TFT_WRITE_COMMAND(0xC4);
     TFT_WRITE_DATA( 0x8D);
     TFT_WRITE_DATA( 0x6A);

     TFT_WRITE_COMMAND(0xC5); //VCOM
     TFT_WRITE_DATA(0x0E);

     TFT_WRITE_COMMAND( 0xE0);
     TFT_WRITE_DATA( 0x10);
     TFT_WRITE_DATA( 0x0E);
     TFT_WRITE_DATA( 0x02);
     TFT_WRITE_DATA( 0x03);
     TFT_WRITE_DATA( 0x0E);
     TFT_WRITE_DATA(0x07);
     TFT_WRITE_DATA( 0x02);
     TFT_WRITE_DATA(0x07);
     TFT_WRITE_DATA(0x0A);
     TFT_WRITE_DATA( 0x12);
     TFT_WRITE_DATA(0x27);
     TFT_WRITE_DATA(0x37);
     TFT_WRITE_DATA(0x00);
     TFT_WRITE_DATA(0x0D);
     TFT_WRITE_DATA( 0x0E);
     TFT_WRITE_DATA(0x10);

     TFT_WRITE_COMMAND( 0xE1);
     TFT_WRITE_DATA( 0x10);
     TFT_WRITE_DATA( 0x0E);
     TFT_WRITE_DATA(0x03);
     TFT_WRITE_DATA( 0x03);
     TFT_WRITE_DATA( 0x0F);
     TFT_WRITE_DATA( 0x06);
     TFT_WRITE_DATA( 0x02);
     TFT_WRITE_DATA( 0x08);
     TFT_WRITE_DATA( 0x0A);
     TFT_WRITE_DATA(0x13);
     TFT_WRITE_DATA(0x26);
     TFT_WRITE_DATA( 0x36);
     TFT_WRITE_DATA(0x00);
     TFT_WRITE_DATA(0x0D);
     TFT_WRITE_DATA( 0x0E);
     TFT_WRITE_DATA( 0x10);

     TFT_WRITE_COMMAND( 0x3A); //
     TFT_WRITE_DATA( 0x05);

     TFT_WRITE_COMMAND( 0x36); //
     TFT_WRITE_DATA(0x78);

     TFT_WRITE_COMMAND(0x13); //

     // LCD_SendCmd(hspi, 0xB4); //
     // LCD_SendDat(hspi, 0x00);
     TFT_WRITE_COMMAND( 0x29); //display on
    // LCD_BLK_1();

 	gpio_set_level(IO_BL, 1);
}






//
void Lcd_SetPosPort( uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    int i = 0;
    y_start +=i;
    y_end +=i;
	TFT_WRITE_COMMAND(0x2A);
	TFT_WRITE_DATA((uint8_t)(x_start >> 8));
	TFT_WRITE_DATA((uint8_t)x_start); //1~160����0~159   ʵ�ʴ����Ĳ�����0~159
	TFT_WRITE_DATA((uint8_t)(x_end >> 8));
	TFT_WRITE_DATA((uint8_t)x_end);

    TFT_WRITE_COMMAND(0x2B);
    TFT_WRITE_DATA((uint8_t)(y_start >> 8));
    TFT_WRITE_DATA((uint8_t)(y_start)); //���26��Ӳ���й�
    TFT_WRITE_DATA((uint8_t)(y_end >> 8));
    TFT_WRITE_DATA((uint8_t)(y_end));
    TFT_WRITE_COMMAND(0x2C);
}

//
static void LCD_SendPointPort(uint16_t Color)
{
    uint8_t Dat[2] = {0};
    Dat[0] = Color >> 4 & 0xFF;
    Dat[1] = Color & 0x00FF;
#if 1
    {
    	lcd_16_data(Dat);
 //   	TFT_WRITE_DATA(Dat[0]);
 //   	TFT_WRITE_DATA(Dat[1]);
    }
#else
    {
        LCD_Dat();
        LCD_CS_0();
        HAL_SPI_Transmit(&hspi, Dat, 2, 0xfff);
        LCD_CS_1();
    }
#endif
}


//
void Lcd_ClearPort( uint16_t Color)
{
    uint16_t i;
    uint16_t x_end = 100;
    uint16_t y_end = 100;
    //Lcd_SetPosPort( 0, 0, X_MAX_PIXEL, Y_MAX_PIXEL);
    Lcd_SetPosPort( 0, 0, x_end, y_end);
#if 1 
    for (i = 0; i < x_end*y_end; i++)
    {
    	//lcd_16_data(Color);
    	LCD_SendPointPort(Color);
    }

#else 
    for (i = 0; i < X_MAX_PIXEL * (Y_MAX_PIXEL); i++)
    {
    	//lcd_16_data(Color);
    	LCD_SendPointPort(Color);
    }
#endif  
}




void Lcd_ClearPort_test( uint16_t Color)
{
    uint16_t i;
    uint16_t x_end = 10;
    uint16_t y_end = 10;
    //Lcd_SetPosPort( 0, 0, X_MAX_PIXEL, Y_MAX_PIXEL);
    Lcd_SetPosPort( 0, 0, x_end, y_end);
#if 1 
    for (i = 0; i < x_end*y_end; i++)
    {
    	//lcd_16_data(Color);
    	LCD_SendPointPort(Color);
    }

#else 
    for (i = 0; i < X_MAX_PIXEL * (Y_MAX_PIXEL); i++)
    {
    	//lcd_16_data(Color);
    	LCD_SendPointPort(Color);
    }
#endif  
    
  
}


//
void Lcd_Fill( uint16_t x_start, uint16_t y_start,
		uint16_t x_end, uint16_t y_end, uint16_t Color)
{
    uint16_t i;
    Lcd_SetPosPort( x_start, y_start, x_end, y_end);
    for (i = 0; i < (x_end - x_start +1) * (y_end - y_start + 1); i++)
    {
    	//lcd_16_data(Color);
    	LCD_SendPointPort(Color);
    }
}


void Lcd_Fill_num(uint16_t x_start, uint16_t y_start,
	uint16_t x_end, uint16_t y_end, uint32_t Color_len,uint8_t *Color_buf){
    Lcd_SetPosPort( x_start, y_start, x_end, y_end);
    lcd_spi_data_by_num(Color_buf, Color_len);

}

uint32_t lcd_dev_init(){
	printf("22lcd_init*************************\n");
	printf("111lcd_init*************************\n");
	printf("11lcd_init*************************\n");
	_vspi_init();
    _096lcd();
	return 0;
}

