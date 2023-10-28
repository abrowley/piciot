#include <iostream>
#include <pico/stdlib.h>

int main() {
    setup_default_uart();
    std::cout << "piciot starting" << std::endl;
    return 0;
}