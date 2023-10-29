#include "FreeRTOS.h"
#include "hardware/structs/rosc.h"
#include <string.h>
#include <time.h>
#include <iostream>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "task.h"
#include "piciot_version.h"

#define LED_PIN 15
#define DEBUG_printf printf
#define MQTT_SERVER_HOST "test.mosquitto.org"
#define MQTT_SERVER_PORT 1883
#define SUBSCRIBE_TOPIC "pico_w/recv"
#define PUBLISH_TOPIC "pico_w/test"
#define CLIENT_ID "PicoW"
#define WIFI_RETRY 3
#define DELAY 1000
#ifndef WIFI_SSID
#error "WIFI_SSID should be passed as a variable to cmake =DWIFI_SSID="<YOUR_SSID_HERE>""
#endif
#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD should be passed as a variable to cmake =DWIFI_PASSWORD="<YOUR_PASSWSORD_HERE>""
#endif



typedef struct MQTT_CLIENT_T_ {
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t received;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;

err_t mqtt_connect(MQTT_CLIENT_T *state);

// Perform initialisation
static MQTT_CLIENT_T *mqtt_client_init(void) {
    auto *state = (MQTT_CLIENT_T *) calloc(1, sizeof(MQTT_CLIENT_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return nullptr;
    }
    state->received = 0;
    return state;
}

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    auto *state = (MQTT_CLIENT_T *) callback_arg;
    DEBUG_printf("DNS query for %s finished with resolved addr of %s.\n",name, ip4addr_ntoa(ipaddr));
    state->remote_addr = *ipaddr;
}

void run_dns_lookup(MQTT_CLIENT_T *state) {
    DEBUG_printf("Running DNS query for %s.\n", MQTT_SERVER_HOST);

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(MQTT_SERVER_HOST, &(state->remote_addr), dns_found, state);
    cyw43_arch_lwip_end();

    if (err == ERR_ARG) {
        DEBUG_printf("failed to start DNS query\n");
        return;
    }

    if (err == ERR_OK) {
        DEBUG_printf("no lookup needed\n");
        return;
    }

    while (state->remote_addr.addr == 0) {
        DEBUG_printf("...\n");
        sleep_ms(1);
    }
}

u32_t data_in = 0;
u8_t buffer[1025];
u8_t data_len = 0;

static void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len) {
    DEBUG_printf("mqtt_pub_start_cb: topic %s\n", topic);

    if (tot_len > 1024) {
        DEBUG_printf("Message length exceeds buffer size, discarding");
    } else {
        data_in = tot_len;
        data_len = 0;
    }
}

static void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    if (data_in > 0) {
        data_in -= len;
        memcpy(&buffer[data_len], data, len);
        data_len += len;

        if (data_in == 0) {
            buffer[data_len] = 0;
            DEBUG_printf("Message received: %s\n", &buffer);
        }
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status != 0) {
        DEBUG_printf("Error during connection: err %d.\n", status);
    } else {
        DEBUG_printf("MQTT connected.\n");
    }
}

void mqtt_pub_request_cb(void *arg, err_t err) {
    auto *state = (MQTT_CLIENT_T *) arg;
    DEBUG_printf("mqtt_pub_request_cb: err %d\n", err);
    state->received++;
}

void mqtt_sub_request_cb(void *arg, err_t err) {
    DEBUG_printf("mqtt_sub_request_cb: err %d\n", err);
}

err_t mqtt_publish(MQTT_CLIENT_T *state) {
    char pub_buffer[128];

    sprintf(pub_buffer, R"(({"message":"hello from picow %lu / %lu"}))", state->received, state->counter);

    err_t err;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification.  AWS IoT does not support QoS 2 */
    u8_t retain = 0;
    cyw43_arch_lwip_begin();
    err = mqtt_publish(state->mqtt_client, PUBLISH_TOPIC, pub_buffer, strlen(pub_buffer), qos, retain, mqtt_pub_request_cb,
                       state);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        DEBUG_printf("Publish err: %d\n", err);
    }

    return err;
}

err_t mqtt_connect(MQTT_CLIENT_T *state) {
    struct mqtt_connect_client_info_t ci{};
    err_t err;

    memset(&ci, 0, sizeof(ci));

    ci.client_id = CLIENT_ID;
    ci.client_user = nullptr;
    ci.client_pass = nullptr;
    ci.keep_alive = 0;
    ci.will_topic = nullptr;
    ci.will_msg = nullptr;
    ci.will_retain = 0;
    ci.will_qos = 0;

#if MQTT_TLS

    struct altcp_tls_config *tls_config;

#if defined(CRYPTO_CA) && defined(CRYPTO_KEY) && defined(CRYPTO_CERT)
    DEBUG_printf("Setting up TLS with 2wayauth.\n");
    tls_config = altcp_tls_create_config_client_2wayauth(
        (const u8_t *)ca, 1 + strlen((const char *)ca),
        (const u8_t *)key, 1 + strlen((const char *)key),
        (const u8_t *)"", 0,
        (const u8_t *)cert, 1 + strlen((const char *)cert)
    );
    // set this here as its a niche case at the moment.
    // see mqtt-sni.patch for changes to support this.
    ci.server_name = MQTT_SERVER_HOST;
#elif defined(CRYPTO_CERT)
    DEBUG_printf("Setting up TLS with cert.\n");
    tls_config = altcp_tls_create_config_client((const u8_t *) cert, 1 + strlen((const char *) cert));
#endif

    if (tls_config == NULL) {
        DEBUG_printf("Failed to initialize config\n");
        return -1;
    }

    ci.tls_config = tls_config;
#endif

    const struct mqtt_connect_client_info_t *client_info = &ci;

    err = mqtt_client_connect(state->mqtt_client, &(state->remote_addr), MQTT_SERVER_PORT, mqtt_connection_cb, state,
                              client_info);

    if (err != ERR_OK) {
        DEBUG_printf("mqtt_connect return %d\n", err);
    }

    return err;
}

void mqtt_run(MQTT_CLIENT_T *state) {
    state->mqtt_client = mqtt_client_new();

    state->counter = 0;

    if (state->mqtt_client == nullptr) {
        DEBUG_printf("Failed to create new mqtt client\n");
        return;
    }
    // psa_crypto_init();
    if (mqtt_connect(state) == ERR_OK) {
        absolute_time_t timeout = nil_time;
        bool subscribed = false;
        mqtt_set_inpub_callback(state->mqtt_client, mqtt_pub_start_cb, mqtt_pub_data_cb, 0);

        while (true) {
            absolute_time_t now = get_absolute_time();
            if (is_nil_time(timeout) || absolute_time_diff_us(now, timeout) <= 0) {
                if (mqtt_client_is_connected(state->mqtt_client)) {
                    cyw43_arch_lwip_begin();

                    if (!subscribed) {
                        mqtt_sub_unsub(state->mqtt_client, SUBSCRIBE_TOPIC, 0, mqtt_sub_request_cb, 0, 1);
                        subscribed = true;
                    }

                    if (mqtt_publish(state) == ERR_OK) {
                        if (state->counter != 0) {
                            DEBUG_printf("published %lu\n", state->counter);
                        }
                        timeout = make_timeout_time_ms(DELAY);
                        state->counter++;
                    } // else ringbuffer is full and we need to wait for messages to flush.
                    cyw43_arch_lwip_end();
                } else {
                    // DEBUG_printf(".");
                }
            }
        }
    }
}


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
    DEBUG_printf("piciot version %s starting\n",GIT_VERSION);
    xTaskCreate(vMqttTask, "MQTT", 256, nullptr, 1, nullptr);
    xTaskCreate(vBlinkTask, "LED", 128, nullptr, 1, nullptr);
    vTaskStartScheduler();
    return 0;
}