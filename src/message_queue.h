//
// Created by alexr on 09/12/23.
//

#ifndef PICIOT_MESSAGE_QUEUE_H
#define PICIOT_MESSAGE_QUEUE_H

#define DEBUG_printf printf
#include "FreeRTOS.h"
#include "queue.h"

#define MESSAGE_SIZE 100

typedef struct MSG_QUEUE_T_ {
    QueueHandle_t queue;
} MSG_QUEUE_T;

MSG_QUEUE_T* message_queue_init();

#endif //PICIOT_MESSAGE_QUEUE_H
