cmake_minimum_required(VERSION 3.16)

set(COMPONENTS 
              "main" "wifi_sta" 
              "freertos" "driver" "esp_timer" "esp_log" "nvs" "nvs_flash")

include($ENV{IDF_PATH}/tools/cmake/project.cmake  "components")
project(LockBox)
