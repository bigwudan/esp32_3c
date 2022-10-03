/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_mod.h"
#include "mqtt_mod.h"
#include "bluetooth.h"

#include "nvs_flash.h"

#include "esp_sleep.h"

#include "soc/rtc.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "string.h"

#include <time.h>
#include <sys/time.h>

static const char *TAG = "main_";


static void _deep_sleep_wait(){

    const int wakeup_time_sec = 60;
    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    ESP_LOGI(TAG, "begin sleep");
    esp_deep_sleep_start();
}



esp_err_t example_register_timer_wakeup(void)
{
#define TIMER_WAKEUP_TIME_US    (30 * 1000 * 1000)

    esp_sleep_enable_timer_wakeup(TIMER_WAKEUP_TIME_US);
    ESP_LOGI(TAG, "timer wakeup source is ready");
    return ESP_OK;
}

static void light_sleep_task(void *args)
{
    while (true) {
        printf("Entering light sleep\n");
        /* To make sure the complete line is printed before entering sleep mode,
         * need to wait until UART TX FIFO is empty:
         */
       

        /* Get timestamp before entering sleep */
        //int64_t t_before_us = esp_timer_get_time();

        /* Enter sleep mode */
        esp_light_sleep_start();

        /* Get timestamp after waking up from sleep */
        //int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char* wakeup_reason;
        switch (esp_sleep_get_wakeup_cause()) {
            case ESP_SLEEP_WAKEUP_TIMER:
                wakeup_reason = "timer";
                break;
            case ESP_SLEEP_WAKEUP_GPIO:
                wakeup_reason = "pin";
                break;
            case ESP_SLEEP_WAKEUP_UART:
                wakeup_reason = "uart";
                /* Hang-up for a while to switch and execuse the uart task
                 * Otherwise the chip may fall sleep again before running uart task */
                vTaskDelay(1);
                break;
            default:
                wakeup_reason = "other";
                break;
        }
        printf("Returned from light sleep, reason: %s\n",
                wakeup_reason);

    }
    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT       4
#define EXAMPLE_PIN_NUM_HSYNC          46
#define EXAMPLE_PIN_NUM_VSYNC          3
#define EXAMPLE_PIN_NUM_DE             0
#define EXAMPLE_PIN_NUM_PCLK           9
#define EXAMPLE_PIN_NUM_DATA0          14 // B0
#define EXAMPLE_PIN_NUM_DATA1          13 // B1
#define EXAMPLE_PIN_NUM_DATA2          12 // B2
#define EXAMPLE_PIN_NUM_DATA3          11 // B3
#define EXAMPLE_PIN_NUM_DATA4          10 // B4
#define EXAMPLE_PIN_NUM_DATA5          39 // G0
#define EXAMPLE_PIN_NUM_DATA6          38 // G1
#define EXAMPLE_PIN_NUM_DATA7          45 // G2
#define EXAMPLE_PIN_NUM_DATA8          48 // G3
#define EXAMPLE_PIN_NUM_DATA9          47 // G4
#define EXAMPLE_PIN_NUM_DATA10         21 // G5
#define EXAMPLE_PIN_NUM_DATA11         1  // R0
#define EXAMPLE_PIN_NUM_DATA12         2  // R1
#define EXAMPLE_PIN_NUM_DATA13         42 // R2
#define EXAMPLE_PIN_NUM_DATA14         41 // R3
#define EXAMPLE_PIN_NUM_DATA15         40 // R4
#define EXAMPLE_PIN_NUM_DISP_EN        -1

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              480
#define EXAMPLE_LCD_V_RES              480

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

esp_lcd_panel_handle_t panel_handle = NULL;

static bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{

    return pdTRUE;
}



static void example_lvgl_flush_cb(int x_start, int y_start, int x_end, int y_end,uint8_t *img_data)
{
    // pass the draw buffer to the driver
    //esp_lcd_panel_draw_bitmap(panel_handle, 10, 0, offsetx2 + 1, offsety2 + 1, img_data);
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end +1, y_end+1 , img_data);

}


static void _task_show_rgb(){
    int x_start = 0;
    int y_start = 0;
    int x_end = 0;
    int y_end = 0;

#define MAX_X 480
#define MAX_Y 200

#define TEST_IMG_SIZE (MAX_X * MAX_Y * sizeof(uint16_t))
uint8_t *img = malloc(TEST_IMG_SIZE); 

    uint8_t color_byte = 0xff;

    memset(img, color_byte, TEST_IMG_SIZE);
    x_end = x_start + MAX_X;
    y_end = y_start + MAX_Y;


    example_lvgl_flush_cb(x_start, y_start,x_end, y_end, img );

    if(img) free(img);

}

//初始化LCD
static void _lcd_init(){


#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 1, //43
            .hsync_front_porch = 8, // 8
            .hsync_pulse_width = 2,
            .vsync_back_porch = 15,
            .vsync_front_porch = 12,
            .vsync_pulse_width = 10,
            .flags.pclk_active_neg = true,
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
#if CONFIG_EXAMPLE_DOUBLE_FB
        .flags.double_fb = true,   // allocate double frame buffer
#endif // CONFIG_EXAMPLE_DOUBLE_FB
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = example_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL));

    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif

}

void app_main(void)
{
   
    esp_err_t res;
    ESP_LOGI(TAG, "main run\n");
#if 0
    res = wifi_mod_start();

    if(res != ESP_OK){
        
        ESP_LOGI(TAG, "wifi_mod_init_no_ok");

    }else{

        ESP_LOGI(TAG, "wifi_mod_init_ok");
    }    
    if(res == ESP_OK){

        mqtt_mod_start();
    }

    bluetooth_app();

    example_register_timer_wakeup();
    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 6, NULL);
    //_deep_sleep_wait();
#endif    


    _lcd_init();
    while(1){
        _task_show_rgb();
	    vTaskDelay(1000 / portTICK_PERIOD_MS);
        //blue_send();
        ESP_LOGI(TAG, "main_task");
        

    }
}
