#pragma once

#include "freertos/FreeRTOS.h"

#include <stdint.h>

// C++ headers must be outside extern "C" block
#ifdef __cplusplus
#include <chrono>
#include <thread>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TaskFunction_t)(void*);

static inline BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                                     const char* /*pcName*/,
                                     const uint32_t /*usStackDepth*/,
                                     void* pvParameters,
                                     UBaseType_t /*uxPriority*/,
                                     TaskHandle_t* pxCreatedTask) {
    // For host stubs, we do not create a real thread.
    // Simulate successful creation and optionally run the function synchronously in tests that depend on it.
    if (pxCreatedTask) {
        *pxCreatedTask = (TaskHandle_t) 0x1; // non-null handle
    }
    // Do NOT run pxTaskCode here to avoid unexpected blocking in tests.
    return pdPASS;
}

static inline void vTaskDelete(TaskHandle_t /*xTaskToDelete*/) {
    // No-op in host stub
}

#ifdef __cplusplus
}
// vTaskDelay implementation needs C++ headers, so define outside extern "C"
static inline void vTaskDelay(const TickType_t xTicksToDelay) {
    // Approximate: ticks map to milliseconds via pdMS_TO_TICKS in stubs
    std::this_thread::sleep_for(std::chrono::milliseconds(xTicksToDelay));
}
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
