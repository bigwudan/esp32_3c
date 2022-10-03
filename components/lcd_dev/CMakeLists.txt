idf_component_register(SRCS "lcd_dev.c" "lcd_dev_tp.c"
                    INCLUDE_DIRS "."
					PRIV_REQUIRES driver lvgl__lvgl esp_timer esp_lcd)
