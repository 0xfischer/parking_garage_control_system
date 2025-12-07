#pragma once

typedef void* SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    return (SemaphoreHandle_t)0x1;
}

static inline int xSemaphoreTake(SemaphoreHandle_t s, unsigned int ticks) {
    (void)s; (void)ticks; return 1; // pdTRUE
}

static inline int xSemaphoreGive(SemaphoreHandle_t s) {
    (void)s; return 1; // pdTRUE
}
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
