cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH} AND NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../../external/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(uart_bridge C CXX ASM)
pico_sdk_init()

add_executable(uart_bridge main.c usb_descriptors.c)
target_include_directories(uart_bridge PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(uart_bridge PRIVATE pico_stdlib pico_unique_id hardware_uart
                      pico_bootrom tinyusb_device)
pico_add_extra_outputs(uart_bridge)
