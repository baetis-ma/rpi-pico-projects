cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picocode C CXX ASM)
set(CMAKE_C STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(picocode
     ws2812b.c
)

pico_generate_pio_header(picocode 
     ../ws2812b.pio
)

target_link_libraries(picocode
     pico_stdlib
     hardware_pio     
     hardware_adc     
)

pico_enable_stdio_usb(picocode 1)
pico_enable_stdio_uart(picocode 0)

pico_add_extra_outputs(picocode)

target_include_directories(picocode PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

