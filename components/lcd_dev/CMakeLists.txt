idf_component_register(SRCS "lcd_dev.c" "lcd_dev_tp.c" "test_img_3.c"
                    INCLUDE_DIRS "."
					PRIV_REQUIRES driver pca9535 lvgl__lvgl esp_timer esp_lcd)
