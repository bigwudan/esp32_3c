idf_component_register(SRCS "src/crc.c" "src/radio.c" "src/sx126x-board.c" "src/sx126x.c"
                    INCLUDE_DIRS "inc/."
					PRIV_REQUIRES driver esp_timer)