#pragma once

#include "IGpioInput.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include <functional>
#include <memory>

/**
 * @brief ESP32 implementation of GPIO input with interrupt support
 *
 * Features:
 * - Internal pull-up resistor enabled
 * - Interrupt on both edges (rising and falling)
 * - IRAM-safe interrupt handler
 * - Software debouncing for buttons
 */
class EspGpioInput : public IGpioInput {
  public:
    /**
     * @brief Construct ESP32 GPIO input
     * @param pin GPIO pin number
     * @param debounceMs Debounce time in milliseconds (0 = no debouncing)
     */
    explicit EspGpioInput(gpio_num_t pin, uint32_t debounceMs = 0);
    ~EspGpioInput() override;

    // Prevent copying
    EspGpioInput(const EspGpioInput&) = delete;
    EspGpioInput& operator=(const EspGpioInput&) = delete;

    [[nodiscard]] bool getLevel() const override;
    void setInterruptHandler(std::function<void(bool level)> handler) override;
    void enableInterrupt() override;
    void disableInterrupt() override;

  private:
    static IRAM_ATTR void gpioIsrHandler(void* arg);
    void handleInterrupt();

    gpio_num_t m_pin;
    uint32_t m_debounceMs;
    std::function<void(bool)> m_handler;
    int64_t m_lastInterruptTime;
};
