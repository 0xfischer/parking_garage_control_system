#pragma once

#include <cstdint>
#include "esp_err.h"
#include "driver/gpio.h"

// LEDC modes
typedef enum {
    LEDC_HIGH_SPEED_MODE = 0,
    LEDC_LOW_SPEED_MODE = 1,
} ledc_mode_t;

// LEDC channels
typedef enum {
    LEDC_CHANNEL_0 = 0,
    LEDC_CHANNEL_1 = 1,
    LEDC_CHANNEL_2 = 2,
    LEDC_CHANNEL_3 = 3,
} ledc_channel_t;

// LEDC timers
typedef enum {
    LEDC_TIMER_0 = 0,
    LEDC_TIMER_1 = 1,
} ledc_timer_t;

// LEDC timer resolution
typedef enum {
    LEDC_TIMER_13_BIT = 13,
    LEDC_TIMER_14_BIT = 14,
    LEDC_TIMER_15_BIT = 15,
} ledc_timer_bit_t;

// LEDC clock source
typedef enum {
    LEDC_AUTO_CLK = 0,
} ledc_clk_cfg_t;

// LEDC interrupt types
typedef enum {
    LEDC_INTR_DISABLE = 0,
    LEDC_INTR_FADE_END = 1,
} ledc_intr_type_t;

// LEDC duty direction
typedef enum {
    LEDC_DUTY_DIR_DECREASE = 0,
    LEDC_DUTY_DIR_INCREASE = 1,
} ledc_duty_direction_t;

// LEDC timer configuration
typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_bit_t duty_resolution;
    ledc_timer_t timer_num;
    uint32_t freq_hz;
    ledc_clk_cfg_t clk_cfg;
    bool deconfigure;
} ledc_timer_config_t;

// LEDC channel configuration
typedef struct {
    int gpio_num;
    ledc_mode_t speed_mode;
    ledc_channel_t channel;
    ledc_intr_type_t intr_type;
    ledc_timer_t timer_sel;
    uint32_t duty;
    int hpoint;
    union {
        uint32_t output_invert;
    } flags;
} ledc_channel_config_t;

// LEDC functions (stubs)
inline esp_err_t ledc_timer_config(const ledc_timer_config_t* timer_conf) {
    return ESP_OK;
}

inline esp_err_t ledc_channel_config(const ledc_channel_config_t* ledc_conf) {
    return ESP_OK;
}

inline esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty) {
    return ESP_OK;
}

inline esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel) {
    return ESP_OK;
}

inline esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level) {
    return ESP_OK;
}
