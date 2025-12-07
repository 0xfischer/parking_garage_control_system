#pragma once

#include "IGate.h"
#include "EspGpioInput.h"
#include "EspServoOutput.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <memory>

/**
 * @brief Concrete gate implementation using GPIO
 *
 * Creates and manages light barrier, motor, and optional button hardware
 */
class Gate : public IGate {
  public:
    /**
     * @brief Construct a gate with GPIO pins (without button)
     * @param lightBarrierPin GPIO pin for light barrier sensor
     * @param motorPin GPIO pin for barrier motor (servo)
     * @param ledcChannel LEDC channel for servo PWM
     */
    Gate(gpio_num_t lightBarrierPin, gpio_num_t motorPin, ledc_channel_t ledcChannel);

    /**
     * @brief Construct a gate with GPIO pins and button
     * @param buttonPin GPIO pin for entry button
     * @param buttonDebounceMs Button debounce time in milliseconds
     * @param lightBarrierPin GPIO pin for light barrier sensor
     * @param motorPin GPIO pin for barrier motor (servo)
     * @param ledcChannel LEDC channel for servo PWM
     */
    Gate(gpio_num_t buttonPin, uint32_t buttonDebounceMs,
         gpio_num_t lightBarrierPin, gpio_num_t motorPin, ledc_channel_t ledcChannel);

    void open() override;
    void close() override;
    bool isOpen() const override;
    bool isCarDetected() const override;

    /**
     * @brief Check if gate has a button
     */
    bool hasButton() const { return m_button != nullptr; }

    /**
     * @brief Get button for interrupt configuration
     * @return Reference to button (only valid if hasButton() returns true)
     */
    EspGpioInput& getButton() { return *m_button; }

    /**
     * @brief Get light barrier for interrupt configuration
     * @return Reference to light barrier
     */
    EspGpioInput& getLightBarrier() { return *m_lightBarrier; }

  private:
    std::unique_ptr<EspGpioInput> m_button; // Optional
    std::unique_ptr<EspGpioInput> m_lightBarrier;
    std::unique_ptr<EspServoOutput> m_motor;
    bool m_isOpen;
};
