cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(webpage C CXX ASM)
set(CMAKE_C STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(picocode
     2core_tcp_heartrate.c
)

target_link_libraries(picocode
     pico_multicore
     hardware_i2c
     pico_cyw43_arch_lwip_threadsafe_background
     pico_stdlib
)

pico_enable_stdio_usb(picocode 1)
pico_enable_stdio_uart(picocode 0)

pico_add_extra_outputs(picocode)

target_include_directories(picocode PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

