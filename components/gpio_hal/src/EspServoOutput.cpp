#include "EspServoOutput.h"
#include "esp_log.h"

static const char* TAG = "EspServoOutput";

EspServoOutput::EspServoOutput(gpio_num_t pin, ledc_channel_t ledcChannel, bool initialLevel)
    : m_pin(pin)
    , m_channel(ledcChannel)
    , m_currentLevel(initialLevel)
{
    // Configure LEDC timer for servo PWM (50Hz)
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_14_BIT,  // 14-bit resolution for precise control
        .timer_num = LEDC_TIMER_0,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return;
    }

    // Configure LEDC channel for the servo
    ledc_channel_config_t channel_conf = {
        .gpio_num = static_cast<int>(m_pin),
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = m_channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 0
        }
    };

    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        return;
    }

    // Set initial position
    uint32_t initialAngle = initialLevel ? SERVO_ANGLE_OPEN : SERVO_ANGLE_CLOSED;
    uint32_t pulseWidthUs = SERVO_MIN_PULSE_US + (initialAngle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / 180;
    uint32_t duty = (pulseWidthUs * ((1 << 14) - 1)) / SERVO_PERIOD_US;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel);

    ESP_LOGI(TAG, "Servo on GPIO %d (channel %d) configured (initial: %s)",
             pin, ledcChannel, initialLevel ? "OPEN" : "CLOSED");
}

EspServoOutput::~EspServoOutput() {
    // Stop PWM and fade to 0
    ledc_stop(LEDC_LOW_SPEED_MODE, m_channel, 0);
}

void EspServoOutput::setLevel(bool high) {
    m_currentLevel = high;
    setAngle(high ? SERVO_ANGLE_OPEN : SERVO_ANGLE_CLOSED);

    ESP_LOGD(TAG, "Servo GPIO %d: %s (angle: %lu°)",
             m_pin,
             high ? "OPEN" : "CLOSED",
             (unsigned long)(high ? SERVO_ANGLE_OPEN : SERVO_ANGLE_CLOSED));
}

bool EspServoOutput::getLevel() const {
    return m_currentLevel;
}

void EspServoOutput::setAngle(uint32_t angleDegrees) {
    // Clamp angle to 0-180
    if (angleDegrees > 180) {
        angleDegrees = 180;
    }

    // Calculate pulse width in microseconds
    // 0° = 1000us, 180° = 2000us
    uint32_t pulseWidthUs = SERVO_MIN_PULSE_US +
                            (angleDegrees * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / 180;

    // Calculate duty cycle for 14-bit resolution
    // duty = (pulse_width_us / period_us) * (2^14 - 1)
    uint32_t duty = (pulseWidthUs * ((1 << 14) - 1)) / SERVO_PERIOD_US;

    // Set the duty cycle directly
    ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel);
}
