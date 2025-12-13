#pragma once

#include "IEventBus.h"
#include "IGate.h"
#include "ITicketService.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <memory>

/**
 * @brief Exit gate state machine states
 */
enum class ExitGateState {
    Idle,
    ValidatingTicket,
    OpeningBarrier,
    WaitingForCarToPass,
    CarPassing,
    WaitingBeforeClose,
    ClosingBarrier
};

/**
 * @brief Exit gate controller with state machine
 *
 * Handles exit sequence:
 * 1. Car arrives (detected via IGate)
 * 2. Validate ticket (simulated - assumes paid)
 * 3. Open barrier via IGate interface
 * 4. Wait for car to pass through
 * 5. Close barrier via IGate interface
 *
 * Uses pure Dependency Injection - all dependencies are injected via constructor.
 */
class ExitGateController {
  public:
    /**
     * @brief Construct exit gate controller with injected dependencies
     * @param eventBus Event bus for publishing/subscribing
     * @param gate Gate abstraction (barrier + light barrier)
     * @param ticketService Ticket service
     * @param barrierTimeoutMs Barrier open/close timeout in ms
     * @param validationTimeMs Ticket validation simulation time in ms
     */
    ExitGateController(
        IEventBus& eventBus,
        IGate& gate,
        ITicketService& ticketService,
        uint32_t barrierTimeoutMs = 2000,
        uint32_t validationTimeMs = 500);

    ~ExitGateController();

    // Prevent copying
    ExitGateController(const ExitGateController&) = delete;
    ExitGateController& operator=(const ExitGateController&) = delete;

    /**
     * @brief Get current state
     */
    [[nodiscard]] ExitGateState getState() const { return m_state; }

    /**
     * @brief Get state as string
     */
    [[nodiscard]] const char* getStateString() const;

    /**
     * @brief Get gate reference (for debugging/console commands)
     */
    [[nodiscard]] IGate& getGate() { return *m_gate; }

    /**
     * @brief Manually validate ticket (for console commands)
     * @param ticketId Ticket ID to validate
     * @return true if validation successful
     */
    bool validateTicketManually(uint32_t ticketId);

    /**
     * @brief Setup GPIO interrupts
     * Call this after construction to enable hardware interrupts
     */
    void setupGpioInterrupts();

    /**
     * @brief Reset controller to initial state
     * Stops timers, resets state to Idle, clears current ticket, closes barrier
     */
    void reset();

#ifdef UNIT_TEST
    // Test helpers to simulate timer expirations without FreeRTOS timers
    void TEST_forceBarrierTimeout() {
        onBarrierTimeout();
    }
    void TEST_forceValidationTimeout() {
        onValidationTimeout();
    }
#endif

  private:
    void onLightBarrierBlocked(const Event& event);
    void onLightBarrierCleared(const Event& event);
    void onBarrierTimeout();
    void onValidationTimeout();

    void setState(ExitGateState newState);
    void startBarrierTimer();
    void stopBarrierTimer();
    void startValidationTimer();
    void stopValidationTimer();

    static void barrierTimerCallback(TimerHandle_t xTimer);
    static void validationTimerCallback(TimerHandle_t xTimer);

    IEventBus& m_eventBus;
    IGate* m_gate;
    ITicketService& m_ticketService;

    ExitGateState m_state;
    uint32_t m_barrierTimeoutMs;
    uint32_t m_validationTimeMs;
    uint32_t m_currentTicketId;
    TimerHandle_t m_barrierTimer;
    TimerHandle_t m_validationTimer;
};
