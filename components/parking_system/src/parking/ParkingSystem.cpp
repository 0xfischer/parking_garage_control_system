#include "ParkingSystem.h"
#include "esp_log.h"
#include <cstdio>

static const char* TAG = "ParkingSystem";

ParkingSystem::ParkingSystem(const Config& config)
    : m_config(config)
{
    ESP_LOGI(TAG, "Creating ParkingSystem...");
    ESP_LOGI(TAG, "  Capacity: %lu", config.capacity);
    ESP_LOGI(TAG, "  Entry Button: GPIO %d", config.entryButtonPin);
    ESP_LOGI(TAG, "  Entry Light Barrier: GPIO %d", config.entryLightBarrierPin);
    ESP_LOGI(TAG, "  Entry Motor: GPIO %d", config.entryMotorPin);
    ESP_LOGI(TAG, "  Exit Light Barrier: GPIO %d", config.exitLightBarrierPin);
    ESP_LOGI(TAG, "  Exit Motor: GPIO %d", config.exitMotorPin);

    // Create event bus
    m_eventBus = std::make_unique<FreeRtosEventBus>(32);

    // Create GPIO objects
    m_entryButton = std::make_unique<EspGpioInput>(config.entryButtonPin, config.buttonDebounceMs);
    m_entryLightBarrier = std::make_unique<EspGpioInput>(config.entryLightBarrierPin, 0);
    m_entryMotor = std::make_unique<EspServoOutput>(config.entryMotorPin, LEDC_CHANNEL_0, false);
    m_exitLightBarrier = std::make_unique<EspGpioInput>(config.exitLightBarrierPin, 0);
    m_exitMotor = std::make_unique<EspServoOutput>(config.exitMotorPin, LEDC_CHANNEL_1, false);

    // Create ticket service
    m_ticketService = std::make_unique<TicketService>(config.capacity);

    // Create gate controllers
    m_entryGate = std::make_unique<EntryGateController>(
        *m_eventBus,
        *m_entryButton,
        *m_entryLightBarrier,
        *m_entryMotor,
        *m_ticketService,
        config.barrierTimeoutMs
    );

    m_exitGate = std::make_unique<ExitGateController>(
        *m_eventBus,
        *m_exitLightBarrier,
        *m_exitMotor,
        *m_ticketService,
        config.barrierTimeoutMs,
        500  // Validation timeout
    );

    ESP_LOGI(TAG, "ParkingSystem created successfully");
}

void ParkingSystem::initialize() {
    ESP_LOGI(TAG, "Initializing ParkingSystem...");
    setupGpioInterrupts();
    ESP_LOGI(TAG, "ParkingSystem initialized and ready");
}

void ParkingSystem::setupGpioInterrupts() {
    // Setup entry button interrupt
    m_entryButton->setInterruptHandler([this](bool level) {
        // Button pressed when level goes LOW (pull-up resistor)
        EventType eventType = level ? EventType::EntryButtonReleased : EventType::EntryButtonPressed;
        Event event(eventType);
        m_eventBus->publishFromISR(event);
    });
    m_entryButton->enableInterrupt();

    // Setup entry light barrier interrupt
    m_entryLightBarrier->setInterruptHandler([this](bool level) {
        // Barrier blocked when level goes LOW
        EventType eventType = level ? EventType::EntryLightBarrierCleared : EventType::EntryLightBarrierBlocked;
        Event event(eventType);
        m_eventBus->publishFromISR(event);
    });
    m_entryLightBarrier->enableInterrupt();

    // Setup exit light barrier interrupt
    m_exitLightBarrier->setInterruptHandler([this](bool level) {
        // Barrier blocked when level goes LOW
        EventType eventType = level ? EventType::ExitLightBarrierCleared : EventType::ExitLightBarrierBlocked;
        Event event(eventType);
        m_eventBus->publishFromISR(event);
    });
    m_exitLightBarrier->enableInterrupt();

    ESP_LOGI(TAG, "GPIO interrupts configured");
}

void ParkingSystem::getStatus(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) {
        return;
    }

    uint32_t active = m_ticketService->getActiveTicketCount();
    uint32_t capacity = m_ticketService->getCapacity();

    snprintf(buffer, bufferSize,
        "=== Parking System Status ===\n"
        "Capacity: %lu/%lu (%lu free)\n"
        "Entry Gate: %s\n"
        "Exit Gate: %s\n"
        "Entry Button: %s\n"
        "Entry Light Barrier: %s\n"
        "Exit Light Barrier: %s\n"
        "Entry Motor: %s\n"
        "Exit Motor: %s\n",
        active, capacity, capacity - active,
        m_entryGate->getStateString(),
        m_exitGate->getStateString(),
        m_entryButton->getLevel() ? "RELEASED" : "PRESSED",
        m_entryLightBarrier->getLevel() ? "CLEAR" : "BLOCKED",
        m_exitLightBarrier->getLevel() ? "CLEAR" : "BLOCKED",
        m_entryMotor->getLevel() ? "OPEN" : "CLOSED",
        m_exitMotor->getLevel() ? "OPEN" : "CLOSED"
    );
}
