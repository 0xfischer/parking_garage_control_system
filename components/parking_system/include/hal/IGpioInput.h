#pragma once

#include <functional>

/**
 * @brief Interface for GPIO input abstraction
 *
 * Provides hardware abstraction for GPIO inputs with interrupt support.
 * Enables testability through mock implementations.
 */
class IGpioInput {
public:
    virtual ~IGpioInput() = default;

    /**
     * @brief Get current logic level of GPIO input
     * @return true if HIGH, false if LOW
     */
    [[nodiscard]] virtual bool getLevel() const = 0;

    /**
     * @brief Set interrupt handler callback
     * @param handler Function called when interrupt triggers (parameter: current level)
     */
    virtual void setInterruptHandler(std::function<void(bool level)> handler) = 0;

    /**
     * @brief Enable interrupt for this GPIO
     */
    virtual void enableInterrupt() = 0;

    /**
     * @brief Disable interrupt for this GPIO
     */
    virtual void disableInterrupt() = 0;
};
