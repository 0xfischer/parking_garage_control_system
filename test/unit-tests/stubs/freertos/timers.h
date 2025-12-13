#pragma once

#include "freertos/FreeRTOS.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TimerStub {
    void* id;
    TickType_t period_ticks;
    BaseType_t auto_reload;
}* TimerHandle_t;

typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);

static inline TimerHandle_t xTimerCreate(const char* /*pcTimerName*/, TickType_t xTimerPeriodInTicks,
                                         BaseType_t uxAutoReload, void* pvTimerID,
                                         TimerCallbackFunction_t /*pxCallbackFunction*/) {
    TimerHandle_t h = new TimerStub{pvTimerID, xTimerPeriodInTicks, uxAutoReload};
    return h;
}

static inline void vTimerSetTimerID(TimerHandle_t xTimer, void* id) {
    if (xTimer)
        xTimer->id = id;
}

static inline void* pvTimerGetTimerID(TimerHandle_t xTimer) {
    return xTimer ? xTimer->id : nullptr;
}

static inline BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t /*xTicksToWait*/) {
    delete xTimer;
    return pdPASS;
}

static inline BaseType_t xTimerReset(TimerHandle_t /*xTimer*/, TickType_t /*xTicksToWait*/) {
    return pdPASS;
}

static inline BaseType_t xTimerStop(TimerHandle_t /*xTimer*/, TickType_t /*xTicksToWait*/) {
    return pdPASS;
}

static inline BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer) {
    // In host stubs, we just return false (timer not active)
    return xTimer ? pdFALSE : pdFALSE;
}

static inline BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, TickType_t xNewPeriod, TickType_t /*xTicksToWait*/) {
    if (xTimer) {
        xTimer->period_ticks = xNewPeriod;
    }
    return pdPASS;
}

#ifdef __cplusplus
}
#endif
