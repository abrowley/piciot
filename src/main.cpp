#include <iostream>
#include <pico/stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN 15

void vBlinkTask(void*) {
    for (;;) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(250);
        gpio_put(LED_PIN, 0);
        vTaskDelay(250);
    }
}

int main() {
    setup_default_uart();
    std::cout << "piciot starting" << std::endl;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN,GPIO_OUT);
    xTaskCreate(vBlinkTask, "Blink Task", 128, nullptr, 1, nullptr);
    vTaskStartScheduler();
    return 0;
}