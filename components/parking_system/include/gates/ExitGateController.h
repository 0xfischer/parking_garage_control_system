#pragma once

#include "IEventBus.h"
#include "IGpioInput.h"
#include "IGpioOutput.h"
#include "ITicketService.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

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
 * 1. Car arrives (light barrier blocked)
 * 2. Validate ticket (simulated - assumes paid)
 * 3. Open barrier
 * 4. Wait for car to pass through
 * 5. Close barrier
 */
class ExitGateController {
public:
    /**
     * @brief Construct exit gate controller
     * @param eventBus Event bus for publishing/subscribing
     * @param lightBarrier Exit light barrier input
     * @param motor Barrier motor output
     * @param ticketService Ticket service
     * @param barrierTimeoutMs Barrier open/close timeout in ms
     * @param validationTimeMs Ticket validation simulation time in ms
     */
    ExitGateController(
        IEventBus& eventBus,
        IGpioInput& lightBarrier,
        IGpioOutput& motor,
        ITicketService& ticketService,
        uint32_t barrierTimeoutMs = 2000,
        uint32_t validationTimeMs = 500
    );

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
     * @brief Manually validate ticket (for console commands)
     * @param ticketId Ticket ID to validate
     * @return true if validation successful
     */
    bool validateTicketManually(uint32_t ticketId);

#ifdef UNIT_TEST
    // Test helpers to simulate timer expirations without FreeRTOS timers
    void TEST_forceBarrierTimeout() { onBarrierTimeout(); }
    void TEST_forceValidationTimeout() { onValidationTimeout(); }
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
    IGpioInput& m_lightBarrier;
    IGpioOutput& m_motor;
    ITicketService& m_ticketService;

    ExitGateState m_state;
    uint32_t m_barrierTimeoutMs;
    uint32_t m_validationTimeMs;
    uint32_t m_currentTicketId;
    TimerHandle_t m_barrierTimer;
    TimerHandle_t m_validationTimer;
};
