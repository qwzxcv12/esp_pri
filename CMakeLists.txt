cmake_minimum_required(VERSION 3.16)
add_compile_definitions("esp_wifi_set_band_mode=(int)" "WIFI_BAND_MODE_2G_ONLY=0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(PRI_ESP)
