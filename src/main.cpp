#include <iostream>
#include <pico/stdlib.h>

int main() {
    setup_default_uart();
    std::cout << "piciot starting" << std::endl;
    while (true){
        std::cout << "Alive " << std::endl;
        sleep_ms(1000);
    }
    return 0;
}