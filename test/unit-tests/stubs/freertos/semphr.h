#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SemaphoreStub {
    int dummy;
}* SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    return new SemaphoreStub{0};
}

static inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {
    delete xSemaphore;
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t /*xSemaphore*/, TickType_t /*xTicksToWait*/) {
    return pdPASS;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t /*xSemaphore*/) {
    return pdPASS;
}

#ifdef __cplusplus
}
#endif
