#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPIO_NUM_0 = 0,
    GPIO_NUM_1 = 1,
    GPIO_NUM_2 = 2,
    GPIO_NUM_3 = 3,
    GPIO_NUM_4 = 4,
    GPIO_NUM_5 = 5,
    // Add more as needed
    GPIO_NUM_MAX = 40
} gpio_num_t;

typedef enum {
    GPIO_MODE_DISABLE = 0,
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2,
    GPIO_MODE_OUTPUT_OD = 3,
    GPIO_MODE_INPUT_OUTPUT_OD = 4,
    GPIO_MODE_INPUT_OUTPUT = 5,
} gpio_mode_t;

typedef enum {
    GPIO_PULLUP_DISABLE = 0,
    GPIO_PULLUP_ENABLE = 1,
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_PULLDOWN_ENABLE = 1,
} gpio_pulldown_t;

typedef enum {
    GPIO_INTR_DISABLE = 0,
    GPIO_INTR_POSEDGE = 1,
    GPIO_INTR_NEGEDGE = 2,
    GPIO_INTR_ANYEDGE = 3,
    GPIO_INTR_LOW_LEVEL = 4,
    GPIO_INTR_HIGH_LEVEL = 5,
} gpio_int_type_t;

typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;

static inline int gpio_config(const gpio_config_t* /*pGPIOConfig*/) {
    return 0;
}

static inline int gpio_set_level(gpio_num_t /*gpio_num*/, uint32_t /*level*/) {
    return 0;
}

static inline int gpio_get_level(gpio_num_t /*gpio_num*/) {
    return 0;
}

static inline int gpio_set_intr_type(gpio_num_t /*gpio_num*/, gpio_int_type_t /*intr_type*/) {
    return 0;
}

static inline int gpio_intr_enable(gpio_num_t /*gpio_num*/) {
    return 0;
}

static inline int gpio_intr_disable(gpio_num_t /*gpio_num*/) {
    return 0;
}

static inline int gpio_isr_handler_add(gpio_num_t /*gpio_num*/, void (*/*fn*/)(void*), void* /*arg*/) {
    return 0;
}

static inline int gpio_isr_handler_remove(gpio_num_t /*gpio_num*/) {
    return 0;
}

static inline int gpio_install_isr_service(int /*intr_alloc_flags*/) {
    return 0;
}

#ifdef __cplusplus
}
#endif
