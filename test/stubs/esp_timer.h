#pragma once

#include <cstdint>
#include <ctime>

static inline int64_t esp_timer_get_time(void) {
    // Return time in microseconds
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
}
