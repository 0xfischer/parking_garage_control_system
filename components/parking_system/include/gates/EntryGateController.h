#pragma once

#include "IEventBus.h"
#include "IGpioInput.h"
#include "IGpioOutput.h"
#include "ITicketService.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

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
 * @brief Entry gate controller with state machine
 *
 * Handles entry sequence:
 * 1. Button press triggers capacity check
 * 2. Issue ticket if capacity available
 * 3. Open barrier
 * 4. Wait for car to pass through
 * 5. Close barrier
 */
class EntryGateController {
public:
    /**
     * @brief Construct entry gate controller
     * @param eventBus Event bus for publishing/subscribing
     * @param button Entry button input
     * @param lightBarrier Entry light barrier input
     * @param motor Barrier motor output
     * @param ticketService Ticket service
     * @param barrierTimeoutMs Barrier open/close timeout in ms
     */
    EntryGateController(
        IEventBus& eventBus,
        IGpioInput& button,
        IGpioInput& lightBarrier,
        IGpioOutput& motor,
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
    IGpioInput& m_button;
    IGpioInput& m_lightBarrier;
    IGpioOutput& m_motor;
    ITicketService& m_ticketService;

    EntryGateState m_state;
    uint32_t m_barrierTimeoutMs;
    uint32_t m_currentTicketId;
    TimerHandle_t m_barrierTimer;
};
