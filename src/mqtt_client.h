//
// Created by alexr on 04/11/23.
//

#ifndef PICIOT_MQTT_CLIENT_H
#define PICIOT_MQTT_CLIENT_H
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"

#define LED_PIN 15
#define DEBUG_printf printf
#define MQTT_SERVER_HOST "mqtt.chiltern-analytics.co.uk"
#define MQTT_SERVER_PORT 1883
#define SUBSCRIBE_TOPIC "pico_w/recv"
#define PUBLISH_TOPIC "pico_w/test"
#define CLIENT_ID "PicoW"

typedef struct MQTT_CLIENT_T_ {
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t received;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;

[[maybe_unused]] err_t mqtt_connect(MQTT_CLIENT_T *state);
MQTT_CLIENT_T* mqtt_client_init();
void run_dns_lookup(MQTT_CLIENT_T *state) ;
void mqtt_run(MQTT_CLIENT_T *state) ;

[[maybe_unused]] void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
[[maybe_unused]] static void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len);
[[maybe_unused]] static void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
[[maybe_unused]] static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
[[maybe_unused]] void mqtt_pub_request_cb(void *arg, err_t err);
[[maybe_unused]] void mqtt_sub_request_cb(void *arg, err_t err) ;
[[maybe_unused]] err_t mqtt_publish(MQTT_CLIENT_T *state);

#endif //PICIOT_MQTT_CLIENT_H
