cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)   # copy from $PICO_SDK_PATH/external/pico_sdk_import.cmake
project(loadcell_keyboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(loadcell_keyboard main.c usb_descriptors.c)
target_include_directories(loadcell_keyboard PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(loadcell_keyboard pico_stdlib pico_unique_id tinyusb_device tinyusb_board)
pico_add_extra_outputs(loadcell_keyboard)
