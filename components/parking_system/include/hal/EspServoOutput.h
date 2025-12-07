#pragma once

#include "IGpioOutput.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/**
 * @brief ESP32 implementation of servo control via PWM
 *
 * Controls a servo motor using LEDC PWM.
 * - LOW (false) = Servo at 90° (barrier closed - vertical)
 * - HIGH (true) = Servo at 0° (barrier open - horizontal)
 */
class EspServoOutput : public IGpioOutput {
  public:
    /**
     * @brief Construct ESP32 Servo output
     * @param pin GPIO pin number
     * @param ledcChannel LEDC channel to use (0-7)
     * @param initialLevel Initial position (false=closed, true=open)
     */
    explicit EspServoOutput(gpio_num_t pin, ledc_channel_t ledcChannel, bool initialLevel = false);
    ~EspServoOutput() override;

    // Prevent copying
    EspServoOutput(const EspServoOutput&) = delete;
    EspServoOutput& operator=(const EspServoOutput&) = delete;

    void setLevel(bool high) override;
    [[nodiscard]] bool getLevel() const override;

  private:
    void setAngle(uint32_t angleDegrees);

    gpio_num_t m_pin;
    ledc_channel_t m_channel;
    bool m_currentLevel;

    // Servo constants
    static constexpr uint32_t SERVO_FREQ_HZ = 50;        // 50Hz for standard servos
    static constexpr uint32_t SERVO_PERIOD_US = 20000;   // 20ms period
    static constexpr uint32_t SERVO_MIN_PULSE_US = 1000; // 1ms = 0°
    static constexpr uint32_t SERVO_MAX_PULSE_US = 2000; // 2ms = 180°
    static constexpr uint32_t SERVO_ANGLE_CLOSED = 90;   // Barrier closed (vertical)
    static constexpr uint32_t SERVO_ANGLE_OPEN = 0;      // Barrier open (horizontal)
};
