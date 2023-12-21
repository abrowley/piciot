#include "FreeRTOS.h"
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include "lwip/altcp_tcp.h"
#include "task.h"
#include "piciot_version.h"
#include "mqtt_client.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "message_queue.h"

extern "C" {
#include "ssd1306.h"
}


#define LED_PIN 15
#define POT_PIN 26
#define DEBUG_printf printf
#define WIFI_RETRY 3
#define DELAY 1000

void init_wifi();

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

void vPotTask(void *mq){
    adc_init();
    gpio_init(POT_PIN);
    adc_select_input(0);
    auto message_queue = static_cast<MSG_QUEUE_T*>(mq);
    char message[MESSAGE_SIZE];
    for(;;){
        float voltage = adc_read() * 3.3f / (1 << 12);
        sprintf(message,R"({"volts": "%f"})",voltage);
        DEBUG_printf(message);
        xQueueSend(message_queue->input_queue,message,(TickType_t)10);
        vTaskDelay(DELAY);
    }
}

void vMqttTask(void * mq) {
    init_wifi();
    auto message_queue = static_cast<MSG_QUEUE_T*>(mq);
    MQTT_CLIENT_T *state = mqtt_client_init();
    state->mq = message_queue;
    run_dns_lookup(state);
    mqtt_run(state);
}

void init_wifi() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        DEBUG_printf("failed to initialise\n");
    }
    cyw43_arch_enable_sta_mode();

    int retry_count = WIFI_RETRY;
    DEBUG_printf("Connecting to WiFi : %s...\n", WIFI_SSID);
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
}

void setup_gpios(void) {
    i2c_init(i2c1, 400000);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);
}

void permute_rows(char* rows[], int len_rows){
    for(int i=1;i!=len_rows;i++){
        memcpy(rows[i-1],rows[i],MESSAGE_SIZE);
    }
}

void vDisplayTask(void * mq) {
    auto message_queue = static_cast<MSG_QUEUE_T*>(mq);

    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, (0x78 >> 1), i2c1);
    ssd1306_clear(&disp);
    ssd1306_show(&disp);

    DEBUG_printf("ANIMATION!\n");
    char data[MESSAGE_SIZE];
    char row0[MESSAGE_SIZE], row1[MESSAGE_SIZE], row2[MESSAGE_SIZE], row3[MESSAGE_SIZE];
    char* display_rows[4] = {row0,row1,row2,row3};
    for (;;) {
        if(xQueueReceive(message_queue->input_queue,data,(TickType_t)10)==pdTRUE){
            ssd1306_draw_string(&disp, 0, 0, 1, "Messages");
            ssd1306_draw_string(&disp, 80, 0, 1, "piciot");
            ssd1306_draw_square(&disp,0,10,150,5);
            display_rows[3] = data;
            for(int i=0;i!=4;i++) {
                ssd1306_draw_string(&disp, 0, 18 + i * (10), 1, display_rows[i]);
            }
            ssd1306_show(&disp);
            xQueueSend(message_queue->output_queue,data,(TickType_t)10);
            permute_rows(display_rows,4);
            ssd1306_clear(&disp);
            vTaskDelay(DELAY/10);
        }else{
            vTaskDelay(DELAY/10);
        }
    }
}


int main() {

    sleep_ms(10000);
    stdio_init_all();
    setup_gpios();


    auto mq = message_queue_init();



    DEBUG_printf("piciot version %s starting\n", PICIOT_VERSION);
    xTaskCreate(
            vMqttTask,
            "MQTT",
            256,
            mq,
            1,
            nullptr
            );

    xTaskCreate(
            vBlinkTask,
            "LED",
            128,
            nullptr,
            1,
            nullptr
            );

    xTaskCreate(
            vDisplayTask,
            "DISPLAY",
            256,
            mq,
            1,
            nullptr
            );

    xTaskCreate(
            vPotTask,
            "POT",
            128,
            mq,
            1,
            nullptr
    );
    vTaskStartScheduler();
    return 0;
}