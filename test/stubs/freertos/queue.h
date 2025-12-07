#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QueueStub {
    int dummy;
}* QueueHandle_t;

static inline QueueHandle_t xQueueCreate(UBaseType_t /*uxQueueLength*/, UBaseType_t /*uxItemSize*/) {
    return new QueueStub{0};
}

static inline void vQueueDelete(QueueHandle_t xQueue) {
    delete xQueue;
}

static inline BaseType_t xQueueSend(QueueHandle_t /*xQueue*/, const void* /*pvItemToQueue*/, TickType_t /*xTicksToWait*/) {
    return pdPASS;
}

static inline BaseType_t xQueueReceive(QueueHandle_t /*xQueue*/, void* /*pvBuffer*/, TickType_t /*xTicksToWait*/) {
    return pdFALSE; // No items in stub queue
}

static inline BaseType_t xQueueSendFromISR(QueueHandle_t /*xQueue*/, const void* /*pvItemToQueue*/, BaseType_t* /*pxHigherPriorityTaskWoken*/) {
    return pdPASS;
}

#ifdef __cplusplus
}
#endif
