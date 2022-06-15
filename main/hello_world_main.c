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
#include "esp_system.h"
#include "esp_spi_flash.h"

//spi lcd
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "lcd_dev.h"
#include <string.h>
#include <stdio.h>

#include "lvgl.h"
#include "../components/lvgl/lvgl_driver/lv_port_disp.h"
static esp_timer_handle_t lvgl_timer_handle = NULL;
static IRAM_ATTR void lv_timer_cb(void *arg){
    lv_tick_inc(1);
}

static esp_timer_create_args_t lvgl_timer = {
    .callback = &lv_timer_cb,
    .arg = NULL,
    .name = "lvgl_timer",
    .dispatch_method = ESP_TIMER_TASK

};


void _lv_timer_create(void){

    esp_err_t err = esp_timer_create(&lvgl_timer, &lvgl_timer_handle);
    err = esp_timer_start_periodic(lvgl_timer_handle, 1000);
}

#if 0
void _fill_buf(uint8_t *t_buf, uint32_t len,uint16_t Color){
    uint8_t Dat[2] = {0};
    Dat[0] = Color >> 4 & 0xFF;
    Dat[1] = Color & 0x00FF;

    for(int i=0; i< len; i++){
        if(i%2 == 0){
            t_buf[i] = Dat[0];

        }else{

            t_buf[i] = Dat[1];

        }
        

    }


    //memset((uint32_t *)t_buf, (uint32_t *)Dat, len/2);

}




uint8_t _t_buf[SPI_SPEED_BUF_NUM] = {0};

static void _fill_rect(uint16_t Color){
    uint16_t x_start = 0; 
    uint16_t y_start =0;
	uint16_t x_end =0; 
    uint16_t y_end = 0;

    int tot_flush = Y_MAX_PIXEL/SPI_SPEED_RAM; //总共需要刷新多少次
    x_end  =X_MAX_PIXEL;
    uint32_t t_len = SPI_SPEED_BUF_NUM;

    _fill_buf(_t_buf, t_len, Color);

    // for(int i=0; i < t_len; i++){
    //     printf("[%d:0x%2X]", i,_t_buf[i]);

    // }
    // printf("end\n");



    for(int i=0; i<t_len; i++){
        y_start = SPI_SPEED_RAM*i;
        y_end = y_start + SPI_SPEED_RAM - 1;
        //printf("poin:[%02X][%02X][%02X][%02X]l[%02X]\n",x_start, y_start, x_end, y_end, t_len);
        Lcd_Fill_num(x_start, y_start, x_end, y_end, t_len, _t_buf);
        
        
    }

}

#endif

    lv_obj_t * line1;
    lv_obj_t * line2;
    lv_obj_t * line3;

void lv_example_line_1(void)
{
    /*Create an array for the points of the line*/
    static lv_point_t line_points[] = { {0, 0}, {100, 0} };
    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 30);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&style_line, true);
    /*Create a line and apply the new style*/
    lv_obj_t * line1;
    line1 = lv_line_create(lv_scr_act());
    lv_line_set_points(line1, line_points, 2);     /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
    lv_obj_center(line1);
}

void lv_example_line_2(void)
{
    /*Create an array for the points of the line*/
    static lv_point_t line_points[] = { {15, 15}, {100, 100} };
    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_rounded(&style_line, true);
    /*Create a line and apply the new style*/
    //lv_obj_t * line1;
    line2 = lv_line_create(lv_scr_act());
    lv_line_set_points(line2, line_points, 2);     /*Set the points*/
    lv_obj_add_style(line2, &style_line, 0);
    lv_obj_center(line2);
}

void lv_example_line_3(void)
{
    /*Create an array for the points of the line*/
    static lv_point_t line_points[] = { {150, 150}, {200, 200} };
    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 5);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_line_rounded(&style_line, true);
    /*Create a line and apply the new style*/
    //lv_obj_t * line1;
    line3 = lv_line_create(lv_scr_act());
    lv_line_set_points(line3, line_points, 2);     /*Set the points*/
    lv_obj_add_style(line3, &style_line, 0);
    lv_obj_center(line3);
}

lv_obj_t * img ;
LV_IMG_DECLARE(test_img_1);
LV_IMG_DECLARE(test_img_2);
static void _test_task(){
    static int idx =0 ;

    if(idx%2 == 0){


        lv_img_set_src(img, &test_img_2);
    }else{

        lv_img_set_src(img, &test_img_1);
    }
    idx++;

}

lv_obj_t *t_obj;
void _my_task(){
    static int idx = 0;
    if(idx%2 == 0){
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 11);
 lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN); 


    }else{
       // lv_obj_set_style_local_bg_color(t_obj, 0, 0, 22);
         lv_obj_set_style_bg_color(t_obj,lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN); 

    }

    idx++;

}

void app_main(void)
{
 

    printf("Restarting now.\n");

    lcd_dev_init();
#if 0  
  
    Lcd_Fill( 0, 0, 240, 240, BLACK);

    int idx = 0;
    while(1){
        idx++;
        if(idx%3 == 0){

            _fill_rect(RED);
        }else if(idx%3 == 1){

            _fill_rect(YELLOW);
        }else if(idx%3 == 2){

            _fill_rect(BLUE);
        }
        


    }
    
#endif
#if 0

    uint32_t t_len = SPI_SPEED_BUF_NUM;

    _fill_buf(_t_buf, t_len, RED);

    for(int i=0; i < t_len; i++){
        printf("[%d:0x%2X]", i,_t_buf[i]);

    }
    printf("end\n");

    Lcd_Fill( 0, 0, 240, 240, BLACK);
    //Lcd_Fill( 0, 0, 0, 240,  RED);
    Lcd_Fill_num(0, 0, 240, 240, t_len, _t_buf);
    while(1);

#endif

#if 1



    lv_init();
    lv_port_disp_init();
    _lv_timer_create();

#if 0
    static lv_obj_t *default_src;
    default_src = lv_scr_act();
    lv_obj_t* label = lv_obj_create(default_src);

    lv_example_line_1();
    img = lv_img_create(lv_scr_act());
    //lv_img_set_src(img, &test_img_1);
    //Lcd_Fill(0, 0,240,240, RED);

    //task = lv_task_create(my_task, 500, LV_TASK_PRIO_MID, NULL);
#else

    t_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(t_obj, 240,240);
    lv_obj_set_pos(t_obj, 0, 0);
    //lv_task_create(_my_task, 500, LV_TASK_PRIO_MID, NULL);
#endif

    while(1){
	    vTaskDelay(2000 / portTICK_PERIOD_MS);
        lv_task_handler();
        //_test_task();
        _my_task();

    }
#endif


}
