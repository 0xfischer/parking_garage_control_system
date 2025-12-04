#pragma once

#include "IGpioOutput.h"
#include "driver/gpio.h"

/**
 * @brief ESP32 implementation of GPIO output
 *
 * Simple GPIO output control for barrier motors and other actuators.
 */
class EspGpioOutput : public IGpioOutput {
public:
    /**
     * @brief Construct ESP32 GPIO output
     * @param pin GPIO pin number
     * @param initialLevel Initial output level (default: LOW)
     */
    explicit EspGpioOutput(gpio_num_t pin, bool initialLevel = false);
    ~EspGpioOutput() override = default;

    // Prevent copying
    EspGpioOutput(const EspGpioOutput&) = delete;
    EspGpioOutput& operator=(const EspGpioOutput&) = delete;

    void setLevel(bool high) override;
    [[nodiscard]] bool getLevel() const override;

private:
    gpio_num_t m_pin;
    bool m_currentLevel;
};
