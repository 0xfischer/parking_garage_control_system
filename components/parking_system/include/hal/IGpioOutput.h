#pragma once

/**
 * @brief Interface for GPIO output abstraction
 *
 * Provides hardware abstraction for GPIO outputs.
 * Enables testability through mock implementations.
 */
class IGpioOutput {
  public:
    virtual ~IGpioOutput() = default;

    /**
     * @brief Set GPIO output level
     * @param high true for HIGH, false for LOW
     */
    virtual void setLevel(bool high) = 0;

    /**
     * @brief Get current GPIO output level
     * @return true if HIGH, false if LOW
     */
    [[nodiscard]] virtual bool getLevel() const = 0;
};
