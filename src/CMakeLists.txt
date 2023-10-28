add_executable(
        ${project_name}
        main.cpp)

pico_enable_stdio_uart(${project_name} 1)
pico_add_extra_outputs(${project_name})
