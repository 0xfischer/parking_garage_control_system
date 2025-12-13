#pragma once

// Stub for host tests - simulate older ESP-IDF version
#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(5, 2, 0)
