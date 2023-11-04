#include "FreeRTOS.h"
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include "lwip/altcp_tcp.h"
#include "task.h"
#include "piciot_version.h"
#include "mqtt_client.h"

#define LED_PIN 15
#define DEBUG_printf printf
#define WIFI_RETRY 3
#define DELAY 1000
#ifndef WIFI_SSID
#error "WIFI_SSID should be passed as a variable to cmake =DWIFI_SSID="<YOUR_SSID_HERE>""
#endif
#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD should be passed as a variable to cmake =DWIFI_PASSWORD="<YOUR_PASSWSORD_HERE>""
#endif


void vBlinkTask(void *) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    for (;;) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(DELAY);
        gpio_put(LED_PIN, 0);
        vTaskDelay(DELAY);
    }
}

void vMqttTask(void *) {
    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
    }
    cyw43_arch_enable_sta_mode();

    int retry_count = WIFI_RETRY;
    DEBUG_printf("Connecting to WiFi : %s...\n",WIFI_SSID);
    while (
            cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)
            && retry_count > 0
            ) {
        DEBUG_printf("failed to  connect retrying, %i of %i attempts left\n", retry_count, WIFI_RETRY);
        retry_count--;
    }
    if (retry_count > 0) {
        DEBUG_printf("Connected.\n");
    } else {
        DEBUG_printf("Connect failed after %i retry attempts\n", WIFI_RETRY);
        exit(-1);
    }

    MQTT_CLIENT_T *state = mqtt_client_init();
    run_dns_lookup(state);
    mqtt_run(state);
}

int main() {
    stdio_init_all();
    DEBUG_printf("piciot version %s starting\n",PICIOT_VERSION);
    xTaskCreate(vMqttTask, "MQTT", 256, nullptr, 1, nullptr);
    xTaskCreate(vBlinkTask, "LED", 128, nullptr, 1, nullptr);
    vTaskStartScheduler();
    return 0;
}