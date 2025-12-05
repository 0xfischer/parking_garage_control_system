#pragma once

#include <cstdint>
#include "esp_err.h"

// GPIO number type
typedef enum {
    GPIO_NUM_0 = 0,
    GPIO_NUM_15 = 15,
    GPIO_NUM_22 = 22,
    GPIO_NUM_25 = 25,
    GPIO_NUM_26 = 26,
    GPIO_NUM_27 = 27,
} gpio_num_t;

// GPIO modes
typedef enum {
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2,
    GPIO_MODE_DISABLE = 0,
} gpio_mode_t;

// GPIO pull modes
typedef enum {
    GPIO_PULLUP_DISABLE = 0,
    GPIO_PULLUP_ENABLE = 1,
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_PULLDOWN_ENABLE = 1,
} gpio_pulldown_t;

// GPIO interrupt types
typedef enum {
    GPIO_INTR_DISABLE = 0,
    GPIO_INTR_POSEDGE = 1,
    GPIO_INTR_NEGEDGE = 2,
    GPIO_INTR_ANYEDGE = 3,
    GPIO_INTR_LOW_LEVEL = 4,
    GPIO_INTR_HIGH_LEVEL = 5,
} gpio_int_type_t;

// GPIO configuration structure
typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;

// GPIO ISR handler type
typedef void (*gpio_isr_t)(void*);

// GPIO functions (stubs)
inline esp_err_t gpio_config(const gpio_config_t* config) {
    return ESP_OK;
}

inline int gpio_get_level(gpio_num_t gpio_num) {
    return 0; // Default level LOW
}

inline esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    return ESP_OK;
}

inline esp_err_t gpio_install_isr_service(int intr_alloc_flags) {
    return ESP_OK;
}

inline esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void* args) {
    return ESP_OK;
}

inline esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num) {
    return ESP_OK;
}
