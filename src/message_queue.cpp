//
// Created by alexr on 09/12/23.
//
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include "message_queue.h"


MSG_QUEUE_T *message_queue_init() {
    auto *msg_queue = static_cast<MSG_QUEUE_T *>(calloc(1, sizeof(MSG_QUEUE_T)));
    if (!msg_queue) {
        DEBUG_printf("failed to allocate queue\n");
        return nullptr;
    }
    msg_queue->queue = xQueueCreate(10,sizeof (char[MESSAGE_SIZE]));
    return msg_queue;
}