#pragma once

#include "IEventBus.h"
#include "IGpioInput.h"
#include "IGate.h"
#include "ITicketService.h"
#include "Gate.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <memory>

/**
 * @brief Entry gate state machine states
 */
enum class EntryGateState {
    Idle,
    CheckingCapacity,
    IssuingTicket,
    OpeningBarrier,
    WaitingForCar,
    CarPassing,
    WaitingBeforeClose,
    ClosingBarrier
};

/**
 * @brief Entry gate controller configuration
 */
struct EntryGateConfig {
    gpio_num_t buttonPin;
    uint32_t buttonDebounceMs;
    gpio_num_t lightBarrierPin;
    gpio_num_t motorPin;
    ledc_channel_t ledcChannel;
    uint32_t barrierTimeoutMs;
};

/**
 * @brief Entry gate controller with state machine
 *
 * Handles entry sequence:
 * 1. Button press triggers capacity check
 * 2. Issue ticket if capacity available
 * 3. Open barrier via IGate interface
 * 4. Wait for car to pass through
 * 5. Close barrier via IGate interface
 *
 * Manages its own Gate hardware internally.
 */
class EntryGateController {
public:
    /**
     * @brief Construct entry gate controller with config (production)
     * @param eventBus Event bus for publishing/subscribing
     * @param ticketService Ticket service
     * @param config Gate hardware configuration
     */
    EntryGateController(
        IEventBus& eventBus,
        ITicketService& ticketService,
        const EntryGateConfig& config
    );

    /**
     * @brief Construct entry gate controller with injected dependencies (testing)
     * @param eventBus Event bus for publishing/subscribing
     * @param button Entry button input
     * @param gate Gate abstraction (barrier + light barrier)
     * @param ticketService Ticket service
     * @param barrierTimeoutMs Barrier open/close timeout in ms
     */
    EntryGateController(
        IEventBus& eventBus,
        IGpioInput& button,
        IGate& gate,
        ITicketService& ticketService,
        uint32_t barrierTimeoutMs = 2000
    );

    ~EntryGateController();

    // Prevent copying
    EntryGateController(const EntryGateController&) = delete;
    EntryGateController& operator=(const EntryGateController&) = delete;

    /**
     * @brief Get current state
     */
    [[nodiscard]] EntryGateState getState() const { return m_state; }

    /**
     * @brief Get state as string
     */
    [[nodiscard]] const char* getStateString() const;

    /**
     * @brief Get gate reference (for debugging/console commands)
     * @note Returns concrete Gate& for access to button/light barrier
     */
    [[nodiscard]] Gate& getGate() { return *m_ownedGate; }

    /**
     * @brief Setup GPIO interrupts (only for production constructor)
     */
    void setupGpioInterrupts();

#ifdef UNIT_TEST
    // Test helpers to simulate timer expirations without FreeRTOS timers
    void TEST_forceBarrierTimeout() { onBarrierTimeout(); }
#endif

private:
    void onButtonPressed(const Event& event);
    void onLightBarrierBlocked(const Event& event);
    void onLightBarrierCleared(const Event& event);
    void onBarrierTimeout();

    void setState(EntryGateState newState);
    void startBarrierTimer();
    void stopBarrierTimer();

    static void barrierTimerCallback(TimerHandle_t xTimer);

    IEventBus& m_eventBus;
    IGpioInput* m_button;  // Pointer for optional ownership
    IGate* m_gate;         // Pointer for optional ownership
    ITicketService& m_ticketService;

    // Optional owned hardware (only for production constructor)
    std::unique_ptr<Gate> m_ownedGate;

    EntryGateState m_state;
    uint32_t m_barrierTimeoutMs;
    uint32_t m_currentTicketId;
    TimerHandle_t m_barrierTimer;
};
