#include "ParkingGarageSystem.h"
#include "esp_log.h"
#include <cstdio>

static const char* TAG = "ParkingGarageSystem";

ParkingGarageSystem::ParkingGarageSystem(const ParkingGarageConfig& config)
    : m_config(config)
{
    ESP_LOGI(TAG, "Creating ParkingGarageSystem (Dependency Injection)...");
    ESP_LOGI(TAG, "  Capacity: %lu", config.capacity);
    ESP_LOGI(TAG, "  Entry Button: GPIO %d", config.entryButtonPin);
    ESP_LOGI(TAG, "  Entry Light Barrier: GPIO %d", config.entryLightBarrierPin);
    ESP_LOGI(TAG, "  Entry Motor: GPIO %d", config.entryMotorPin);
    ESP_LOGI(TAG, "  Exit Light Barrier: GPIO %d", config.exitLightBarrierPin);
    ESP_LOGI(TAG, "  Exit Motor: GPIO %d", config.exitMotorPin);

    // 1. Create shared services
    m_eventBus = std::make_unique<FreeRtosEventBus>(32);
    m_ticketService = std::make_unique<TicketService>(config.capacity);

    // 2. Create hardware (owned by ParkingGarageSystem)
    // Entry gate has button + light barrier + motor
    m_entryGateHw = std::make_unique<Gate>(
        config.entryButtonPin,
        config.buttonDebounceMs,
        config.entryLightBarrierPin,
        config.entryMotorPin,
        LEDC_CHANNEL_0
    );

    // Exit gate has only light barrier + motor (no button)
    m_exitGateHw = std::make_unique<Gate>(
        config.exitLightBarrierPin,
        config.exitMotorPin,
        LEDC_CHANNEL_1
    );

    // 3. Create controllers with injected dependencies
    m_entryGate = std::make_unique<EntryGateController>(
        *m_eventBus,
        m_entryGateHw->getButton(),  // Inject button
        *m_entryGateHw,              // Inject gate
        *m_ticketService,
        config.barrierTimeoutMs
    );

    m_exitGate = std::make_unique<ExitGateController>(
        *m_eventBus,
        *m_exitGateHw,               // Inject gate
        *m_ticketService,
        config.barrierTimeoutMs,
        500  // validationTimeMs
    );

    ESP_LOGI(TAG, "ParkingGarageSystem created successfully");
}

void ParkingGarageSystem::initialize() {
    ESP_LOGI(TAG, "Initializing ParkingGarageSystem...");

    // Setup GPIO interrupts for entry gate (button + light barrier)
    m_entryGate->setupGpioInterrupts();

    // Setup light barrier interrupt for exit gate
    m_exitGateHw->getLightBarrier().setInterruptHandler([this](bool level) {
        EventType eventType = level ? EventType::ExitLightBarrierCleared : EventType::ExitLightBarrierBlocked;
        Event event(eventType);
        m_eventBus->publish(event);
    });
    m_exitGateHw->getLightBarrier().enableInterrupt();

    // Setup light barrier interrupt for entry gate
    m_entryGateHw->getLightBarrier().setInterruptHandler([this](bool level) {
        EventType eventType = level ? EventType::EntryLightBarrierCleared : EventType::EntryLightBarrierBlocked;
        Event event(eventType);
        m_eventBus->publish(event);
    });
    m_entryGateHw->getLightBarrier().enableInterrupt();

    m_exitGate->setupGpioInterrupts();

    ESP_LOGI(TAG, "ParkingGarageSystem initialized and ready");
}

void ParkingGarageSystem::getStatus(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) {
        return;
    }

    uint32_t active = m_ticketService->getActiveTicketCount();
    uint32_t capacity = m_ticketService->getCapacity();

    snprintf(buffer, bufferSize,
        "=== Parking System Status ===\n"
        "Capacity: %lu/%lu (%lu free)\n"
        "Entry Gate: %s\n"
        "Exit Gate: %s\n",
        active, capacity, capacity - active,
        m_entryGate->getStateString(),
        m_exitGate->getStateString()
    );
}
