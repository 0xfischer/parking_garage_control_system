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

    // Create ticket service
    m_ticketService = std::make_unique<TicketService>(config.capacity);

    // Create gate controllers (they manage their own Gate hardware)
    m_entryGate = std::make_unique<EntryGateController>(
        *m_eventBus,
        *m_ticketService,
        EntryGateConfig{
            .buttonPin = config.entryButtonPin,
            .buttonDebounceMs = config.buttonDebounceMs,
            .lightBarrierPin = config.entryLightBarrierPin,
            .motorPin = config.entryMotorPin,
            .ledcChannel = LEDC_CHANNEL_0,
            .barrierTimeoutMs = config.barrierTimeoutMs
        }
    );

    m_exitGate = std::make_unique<ExitGateController>(
        *m_eventBus,
        *m_ticketService,
        ExitGateConfig{
            .lightBarrierPin = config.exitLightBarrierPin,
            .motorPin = config.exitMotorPin,
            .ledcChannel = LEDC_CHANNEL_1,
            .barrierTimeoutMs = config.barrierTimeoutMs,
            .validationTimeMs = 500
        }
    );

    ESP_LOGI(TAG, "ParkingSystem created successfully");
}

void ParkingSystem::initialize() {
    ESP_LOGI(TAG, "Initializing ParkingSystem...");

    // Setup GPIO interrupts in the controllers
    m_entryGate->setupGpioInterrupts();
    m_exitGate->setupGpioInterrupts();

    ESP_LOGI(TAG, "ParkingSystem initialized and ready");
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
        "Exit Gate: %s\n",
        active, capacity, capacity - active,
        m_entryGate->getStateString(),
        m_exitGate->getStateString()
    );
}
