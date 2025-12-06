#pragma once

#include "EntryGateController.h"
#include "ExitGateController.h"
#include "FreeRtosEventBus.h"
#include "TicketService.h"
#include "ParkingGarageConfig.h"
#include "Gate.h"
#include <memory>

/**
 * @brief Main parking garage system orchestrator
 *
 * Uses pure Dependency Injection pattern:
 * - Creates all hardware (Gate, Button, LightBarrier, Motor)
 * - Creates all services (EventBus, TicketService)
 * - Injects dependencies into controllers
 *
 * Ownership hierarchy:
 * - ParkingGarageSystem owns: EventBus, TicketService, Gates, Controllers
 * - Controllers receive references to: EventBus, Gate, TicketService
 */
class ParkingGarageSystem {
public:
    /**
     * @brief Construct parking system
     * @param config System configuration
     */
    explicit ParkingGarageSystem(const ParkingGarageConfig& config);
    ~ParkingGarageSystem() = default;

    // Prevent copying
    ParkingGarageSystem(const ParkingGarageSystem&) = delete;
    ParkingGarageSystem& operator=(const ParkingGarageSystem&) = delete;

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
     * @brief Get entry gate hardware reference (for console commands)
     */
    Gate& getEntryGateHardware() { return *m_entryGateHw; }

    /**
     * @brief Get exit gate hardware reference (for console commands)
     */
    Gate& getExitGateHardware() { return *m_exitGateHw; }

    /**
     * @brief Get system status string
     */
    void getStatus(char* buffer, size_t bufferSize) const;

private:
    // Event bus (must be first - other components depend on it)
    std::unique_ptr<FreeRtosEventBus> m_eventBus;

    // Services
    std::unique_ptr<TicketService> m_ticketService;

    // Hardware (owned by ParkingGarageSystem, injected into controllers)
    std::unique_ptr<Gate> m_entryGateHw;
    std::unique_ptr<Gate> m_exitGateHw;

    // Controllers (receive injected dependencies)
    std::unique_ptr<EntryGateController> m_entryGate;
    std::unique_ptr<ExitGateController> m_exitGate;

    ParkingGarageConfig m_config;
};
