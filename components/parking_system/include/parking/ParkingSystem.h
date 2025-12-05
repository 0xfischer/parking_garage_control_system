#pragma once

#include "EntryGateController.h"
#include "ExitGateController.h"
#include "FreeRtosEventBus.h"
#include "TicketService.h"
#include <memory>

/**
 * @brief Main parking system orchestrator
 *
 * Manages all components:
 * - Event bus
 * - Ticket service
 * - Entry gate controller
 * - Exit gate controller
 * - GPIO hardware setup
 */
class ParkingSystem {
public:
    /**
     * @brief Configuration for parking system
     */
    struct Config {
        // GPIO pins
        gpio_num_t entryButtonPin;
        gpio_num_t entryLightBarrierPin;
        gpio_num_t entryMotorPin;
        gpio_num_t exitLightBarrierPin;
        gpio_num_t exitMotorPin;

        // System parameters
        uint32_t capacity;
        uint32_t barrierTimeoutMs;
        uint32_t buttonDebounceMs;
    };

    /**
     * @brief Construct parking system
     * @param config System configuration
     */
    explicit ParkingSystem(const Config& config);
    ~ParkingSystem() = default;

    // Prevent copying
    ParkingSystem(const ParkingSystem&) = delete;
    ParkingSystem& operator=(const ParkingSystem&) = delete;

    /**
     * @brief Initialize system and start event processing
     */
    void initialize();

    /**
     * @brief Get event bus reference
     */
    IEventBus& getEventBus() { return *m_eventBus; }

    /**
     * @brief Get ticket service reference
     */
    ITicketService& getTicketService() { return *m_ticketService; }

    /**
     * @brief Get entry gate controller reference
     */
    EntryGateController& getEntryGate() { return *m_entryGate; }

    /**
     * @brief Get exit gate controller reference
     */
    ExitGateController& getExitGate() { return *m_exitGate; }

    /**
     * @brief Get system status string
     */
    void getStatus(char* buffer, size_t bufferSize) const;

private:
    // Event bus (must be first - other components depend on it)
    std::unique_ptr<FreeRtosEventBus> m_eventBus;

    // Services
    std::unique_ptr<TicketService> m_ticketService;

    // Controllers (own their own Gate hardware)
    std::unique_ptr<EntryGateController> m_entryGate;
    std::unique_ptr<ExitGateController> m_exitGate;

    Config m_config;
};
