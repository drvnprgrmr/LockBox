set(srcs)
set(include_dirs "include")

if(CONFIG_WIFI_STA_CONNECT)
  list(APPEND srcs "wifi_sta.c")
endif(CONFIG_WIFI_STA_CONNECT)


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include}"
                       PRIV_REQUIRES esp_wifi esp_netif)
