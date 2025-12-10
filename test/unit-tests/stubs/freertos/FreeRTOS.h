#pragma once

#include <cstdint>

typedef int BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef void* TaskHandle_t;

#ifndef pdTRUE
#define pdTRUE 1
#endif
#ifndef pdFALSE
#define pdFALSE 0
#endif

#ifndef pdPASS
#define pdPASS 1
#endif

#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFF
#endif

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS (1)
#endif

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (static_cast<TickType_t>(ms))
#endif

#ifndef portYIELD_FROM_ISR
#define portYIELD_FROM_ISR(x) (void) (x)
#endif

#define configMINIMAL_STACK_SIZE (1024)
