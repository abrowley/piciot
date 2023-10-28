cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 23)
set(project_name piciot)
include(lib/pico-sdk/pico_sdk_init.cmake)
project(${project_name} C CXX ASM)
pico_sdk_init()

add_subdirectory(src)
