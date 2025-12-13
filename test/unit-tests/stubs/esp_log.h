#pragma once

#include <cstdio>

#define ESP_LOGI(TAG, FMT, ...) std::printf("I (%s) " FMT "\n", TAG, ##__VA_ARGS__)
#define ESP_LOGD(TAG, FMT, ...) std::printf("D (%s) " FMT "\n", TAG, ##__VA_ARGS__)
#define ESP_LOGW(TAG, FMT, ...) std::printf("W (%s) " FMT "\n", TAG, ##__VA_ARGS__)
#define ESP_LOGE(TAG, FMT, ...) std::printf("E (%s) " FMT "\n", TAG, ##__VA_ARGS__)
