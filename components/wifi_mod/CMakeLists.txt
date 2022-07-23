idf_component_register(SRCS "wifi_mod.c"
                    INCLUDE_DIRS "."
					PRIV_REQUIRES driver nvs_flash bt esp_wifi esp_http_server mqtt lwip)
