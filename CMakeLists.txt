cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)
set(project_name piciot)
set(CMAKE_VERBOSE_MAKEFILE ON)
include(lib/pico-sdk/pico_sdk_init.cmake)
include(lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
project(${project_name} C CXX ASM)
pico_sdk_init()

add_executable(
        ${project_name}
        src/main.cpp)

target_include_directories(${project_name} PRIVATE port/FreeRTOS-Kernel)

target_link_libraries(${project_name}
        pico_stdlib
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
)
pico_enable_stdio_uart(${project_name} 1)
pico_add_extra_outputs(${project_name})
