#include "sdkconfig.h"
#include "parking/ParkingGarageConfig.h"

ParkingGarageConfig::ParkingGarageConfig()
    : entryButtonPin(GPIO_NUM_25)
    , entryLightBarrierPin(GPIO_NUM_15)
    , entryMotorPin(GPIO_NUM_22)
    , exitLightBarrierPin(GPIO_NUM_26)
    , exitMotorPin(GPIO_NUM_27)
    , capacity(5)
    , barrierTimeoutMs(2000)
    , buttonDebounceMs(50) {}

bool ParkingGarageConfig::isValid() const {
    // Check that all pins are different
    if (entryButtonPin == entryLightBarrierPin ||
        entryButtonPin == entryMotorPin ||
        entryButtonPin == exitLightBarrierPin ||
        entryButtonPin == exitMotorPin ||
        entryLightBarrierPin == entryMotorPin ||
        entryLightBarrierPin == exitLightBarrierPin ||
        entryLightBarrierPin == exitMotorPin ||
        entryMotorPin == exitLightBarrierPin ||
        entryMotorPin == exitMotorPin ||
        exitLightBarrierPin == exitMotorPin) {
        return false;
    }

    // Check capacity is reasonable
    if (capacity == 0 || capacity > 1000) {
        return false;
    }

    // Check timeouts are reasonable
    if (barrierTimeoutMs < 100 || barrierTimeoutMs > 10000) {
        return false;
    }

    if (buttonDebounceMs < 10 || buttonDebounceMs > 1000) {
        return false;
    }

    return true;
}

/**
 * @brief Get parking garage system configuration from Kconfig
 */
ParkingGarageConfig ParkingGarageConfig::fromKconfig() {
    ParkingGarageConfig config;

    // GPIO configuration from Kconfig
    config.entryButtonPin = static_cast<gpio_num_t>(CONFIG_PARKING_ENTRY_BUTTON_GPIO);
    config.entryLightBarrierPin = static_cast<gpio_num_t>(CONFIG_PARKING_ENTRY_LIGHT_BARRIER_GPIO);
    config.entryMotorPin = static_cast<gpio_num_t>(CONFIG_PARKING_ENTRY_MOTOR_GPIO);
    config.exitLightBarrierPin = static_cast<gpio_num_t>(CONFIG_PARKING_EXIT_LIGHT_BARRIER_GPIO);
    config.exitMotorPin = static_cast<gpio_num_t>(CONFIG_PARKING_EXIT_MOTOR_GPIO);

    // System configuration from Kconfig
    config.capacity = CONFIG_PARKING_CAPACITY;
    config.barrierTimeoutMs = CONFIG_PARKING_BARRIER_TIMEOUT_MS;
    config.buttonDebounceMs = CONFIG_PARKING_BUTTON_DEBOUNCE_MS;

    return config;
}

// Backward-compatible free function wrapper
ParkingGarageConfig get_system_config() {
    return ParkingGarageConfig::fromKconfig();
}
