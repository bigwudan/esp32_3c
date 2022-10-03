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




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RGB_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)
#define RGB_LCD_BK_LIGHT_ON_LEVEL  1
#define RGB_LCD_BK_LIGHT_OFF_LEVEL !RGB_LCD_BK_LIGHT_ON_LEVEL
#define RGB_PIN_NUM_BK_LIGHT       4
#define RGB_PIN_NUM_HSYNC          46
#define RGB_PIN_NUM_VSYNC          3
#define RGB_PIN_NUM_DE             0
#define RGB_PIN_NUM_PCLK           9
#define RGB_PIN_NUM_DATA0          14 // B0
#define RGB_PIN_NUM_DATA1          13 // B1
#define RGB_PIN_NUM_DATA2          12 // B2
#define RGB_PIN_NUM_DATA3          11 // B3
#define RGB_PIN_NUM_DATA4          10 // B4
#define RGB_PIN_NUM_DATA5          39 // G0
#define RGB_PIN_NUM_DATA6          38 // G1
#define RGB_PIN_NUM_DATA7          45 // G2
#define RGB_PIN_NUM_DATA8          48 // G3
#define RGB_PIN_NUM_DATA9          47 // G4
#define RGB_PIN_NUM_DATA10         21 // G5
#define RGB_PIN_NUM_DATA11         1  // R0
#define RGB_PIN_NUM_DATA12         2  // R1
#define RGB_PIN_NUM_DATA13         42 // R2
#define RGB_PIN_NUM_DATA14         41 // R3
#define RGB_PIN_NUM_DATA15         40 // R4
#define RGB_PIN_NUM_DISP_EN        -1

// The pixel number in horizontal and vertical
#define RGB_LCD_H_RES              480
#define RGB_LCD_V_RES              480

#define LVGL_TICK_PERIOD_MS    2


static const char *TAG = "lcd_dev";

esp_lcd_panel_handle_t panel_handle = NULL;

static void _increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void _lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{

    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

void example_lvgl_demo_ui()
{
  /*Create an Arc*/
  lv_obj_t * arc = lv_arc_create(lv_scr_act());
  lv_obj_set_size(arc, 150, 150);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_value(arc, 40);
  lv_obj_center(arc);
}


static void _lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    tp_data_tag *tp_data_info = lcd_dev_tp_get_data();
    if (tp_data_info->press_state == 1) {
        data->point.x = tp_data_info->x_pos;
        data->point.y = tp_data_info->y_pos;
        data->state = LV_INDEV_STATE_PRESSED;
        
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
       
    }
}

static void _lvgl_init(){
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions    

    uint8_t *img_buf = malloc(RGB_LCD_H_RES * RGB_LCD_V_RES * 2);

    lv_init();
    lv_disp_draw_buf_init(&disp_buf, img_buf, NULL, RGB_LCD_H_RES * RGB_LCD_V_RES);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = RGB_LCD_H_RES;
    disp_drv.ver_res = RGB_LCD_V_RES;
    disp_drv.flush_cb = _lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);    


    //tp


    static lv_indev_drv_t indev_drv;    // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = _lvgl_touch_cb;
    indev_drv.user_data = NULL;

    lv_indev_drv_register(&indev_drv);    


    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));  
    example_lvgl_demo_ui();  

}


static void _rgb_dev_init(){
    ESP_LOGI(TAG, "Install RGB LCD panel driver");


    //bk
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << RGB_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));  
    gpio_set_level(RGB_PIN_NUM_BK_LIGHT, 1);  
    
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,

        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = RGB_PIN_NUM_DISP_EN,
        .pclk_gpio_num = RGB_PIN_NUM_PCLK,
        .vsync_gpio_num = RGB_PIN_NUM_VSYNC,
        .hsync_gpio_num = RGB_PIN_NUM_HSYNC,
        .de_gpio_num = RGB_PIN_NUM_DE,
        .data_gpio_nums = {
            RGB_PIN_NUM_DATA0,
            RGB_PIN_NUM_DATA1,
            RGB_PIN_NUM_DATA2,
            RGB_PIN_NUM_DATA3,
            RGB_PIN_NUM_DATA4,
            RGB_PIN_NUM_DATA5,
            RGB_PIN_NUM_DATA6,
            RGB_PIN_NUM_DATA7,
            RGB_PIN_NUM_DATA8,
            RGB_PIN_NUM_DATA9,
            RGB_PIN_NUM_DATA10,
            RGB_PIN_NUM_DATA11,
            RGB_PIN_NUM_DATA12,
            RGB_PIN_NUM_DATA13,
            RGB_PIN_NUM_DATA14,
            RGB_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = RGB_LCD_PIXEL_CLOCK_HZ,
            .h_res = RGB_LCD_H_RES,
            .v_res = RGB_LCD_V_RES,
            // The following parameters should refer to LCD spec
      
            .hsync_back_porch = 0,//43
            .hsync_front_porch = 8,
            .hsync_pulse_width = 2,
            .vsync_back_porch = 0, //15
            .vsync_front_porch = 12,//12
            .vsync_pulse_width = 10,//10
            .flags.pclk_active_neg = true,

        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM

    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
#if 0
    ESP_LOGI(TAG, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));
#endif
    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));




}






void lcd_dev_init(){
    ESP_LOGI(TAG, "lcd_dev_init");
    _rgb_dev_init();
    _lvgl_init();

    //tp
    lcd_dev_tp_init();
    return ;
}

void lcd_dev_task(){

    lv_timer_handler();
    lcd_dev_tp_scan_tp();
    return ;
}