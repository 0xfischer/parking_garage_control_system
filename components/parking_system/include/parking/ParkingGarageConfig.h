#pragma once

#include "driver/gpio.h"
#include <cstdint>

/**
 * @brief Configuration for the parking garage system
 *
 * Contains all GPIO pin assignments and system parameters needed
 * to configure the parking garage control system.
 */
class ParkingGarageConfig {
  public:
    // GPIO pin assignments
    gpio_num_t entryButtonPin;
    gpio_num_t entryLightBarrierPin;
    gpio_num_t entryMotorPin;
    gpio_num_t exitLightBarrierPin;
    gpio_num_t exitMotorPin;

    // System parameters
    uint32_t capacity;         // Maximum parking capacity
    uint32_t barrierTimeoutMs; // Barrier operation timeout
    uint32_t buttonDebounceMs; // Button debounce time

    /**
     * @brief Default constructor with sensible defaults
     */
    ParkingGarageConfig();

    /**
     * @brief Validate configuration
     * @return true if configuration is valid
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief Create configuration from Kconfig values (factory)
     */
    static ParkingGarageConfig fromKconfig();
};
