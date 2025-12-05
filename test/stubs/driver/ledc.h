#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LEDC_TIMER_0 = 0,
    LEDC_TIMER_1 = 1,
    LEDC_TIMER_2 = 2,
    LEDC_TIMER_3 = 3,
    LEDC_TIMER_MAX
} ledc_timer_t;

typedef enum {
    LEDC_CHANNEL_0 = 0,
    LEDC_CHANNEL_1 = 1,
    LEDC_CHANNEL_2 = 2,
    LEDC_CHANNEL_3 = 3,
    LEDC_CHANNEL_4 = 4,
    LEDC_CHANNEL_5 = 5,
    LEDC_CHANNEL_MAX
} ledc_channel_t;

typedef enum {
    LEDC_HIGH_SPEED_MODE = 0,
    LEDC_LOW_SPEED_MODE = 1,
    LEDC_SPEED_MODE_MAX
} ledc_mode_t;

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_t timer_sel;
    int channel;
    int intr_type;
    int gpio_num;
    int duty;
    int hpoint;
} ledc_channel_config_t;

typedef struct {
    ledc_mode_t speed_mode;
    int duty_resolution;
    ledc_timer_t timer_num;
    int freq_hz;
    int clk_cfg;
} ledc_timer_config_t;

static inline int ledc_channel_config(const ledc_channel_config_t* /*ledc_conf*/) {
    return 0;
}

static inline int ledc_timer_config(const ledc_timer_config_t* /*timer_conf*/) {
    return 0;
}

static inline int ledc_set_duty(ledc_mode_t /*speed_mode*/, ledc_channel_t /*channel*/, uint32_t /*duty*/) {
    return 0;
}

static inline int ledc_update_duty(ledc_mode_t /*speed_mode*/, ledc_channel_t /*channel*/) {
    return 0;
}

#ifdef __cplusplus
}
#endif
