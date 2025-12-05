#pragma once

/**
 * @brief Gate interface for barrier control abstraction
 *
 * This interface abstracts the physical gate components (button, light barrier, motor)
 * allowing the gate controller logic to be independent of hardware specifics.
 */
class IGate {
public:
    virtual ~IGate() = default;

    /**
     * @brief Open the barrier
     */
    virtual void open() = 0;

    /**
     * @brief Close the barrier
     */
    virtual void close() = 0;

    /**
     * @brief Check if barrier is currently open
     * @return true if barrier is open, false otherwise
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief Check if car is detected in light barrier
     * @return true if car is blocking light barrier, false otherwise
     */
    virtual bool isCarDetected() const = 0;
};
