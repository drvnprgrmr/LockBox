set(COMPONENTS "main" "freertos" "driver" "esp_timer" "esp_log" "nvs" "nvs_flash")

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(LockBox)
