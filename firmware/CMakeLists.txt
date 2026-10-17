cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH} AND NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../external/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(elrs_sim_dongle C CXX ASM)
pico_sdk_init()

add_executable(elrs_sim_dongle main.c crsf.c usb_descriptors.c)
target_include_directories(elrs_sim_dongle PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(elrs_sim_dongle PRIVATE pico_stdlib pico_unique_id
                      hardware_uart hardware_irq tinyusb_device)

pico_add_extra_outputs(elrs_sim_dongle)
