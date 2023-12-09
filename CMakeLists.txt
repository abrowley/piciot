cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)
set(project_name piciot)
set(CMAKE_VERBOSE_MAKEFILE ON)
include(lib/pico-sdk/pico_sdk_init.cmake)
include(lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
project(${project_name} C CXX ASM)
pico_sdk_init()

execute_process(COMMAND git describe --tags --always OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE GIT_REPO_VERSION)
configure_file("src/piciot_version.h.in" "piciot_version.h")

add_executable(
        ${project_name}
        src/main.cpp
        src/mqtt_client.cpp
        src/ssd1306.c
)

target_include_directories(${project_name} PRIVATE port/FreeRTOS-Kernel)
target_include_directories(${project_name} PRIVATE port/lwip)
target_include_directories (${project_name} PRIVATE ${CMAKE_BINARY_DIR})

if((DEFINED WIFI_SSID) AND (DEFINED WIFI_PASSWORD))
    target_compile_definitions(${project_name} PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
else()
      message(FATAL_ERROR
              "WIFI Credentials not defined. Add -DWIFI_SSID=<YOUR_SSID> and -DWIFI_PASSWORD=<YOUR_PASSWORD> to CMAKE Args")
endif ()

target_link_libraries(${project_name}
        pico_stdlib
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mqtt
        FreeRTOS-Kernel-Heap4
        hardware_i2c
)

pico_enable_stdio_uart(${project_name} 1)
pico_add_extra_outputs(${project_name})
