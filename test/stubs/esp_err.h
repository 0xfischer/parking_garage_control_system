#pragma once

#include <cstdint>
#include <cstring>

typedef int32_t esp_err_t;

#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG -2
#define ESP_ERR_INVALID_STATE -3

inline const char* esp_err_to_name(esp_err_t error) {
    switch (error) {
        case ESP_OK: return "ESP_OK";
        case ESP_FAIL: return "ESP_FAIL";
        case ESP_ERR_INVALID_ARG: return "ESP_ERR_INVALID_ARG";
        case ESP_ERR_INVALID_STATE: return "ESP_ERR_INVALID_STATE";
        default: return "UNKNOWN_ERROR";
    }
}
