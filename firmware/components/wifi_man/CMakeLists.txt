idf_component_register(SRCS "wifi.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi esp_netif nvs_flash)
