#pragma once

#include "IEventBus.h"
#include "IGate.h"
#include "ITicketService.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <memory>

// Forward declaration
class Gate;

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
 * @brief Configuration for ExitGateController in production mode
 */
struct ExitGateConfig {
    gpio_num_t lightBarrierPin;
    gpio_num_t motorPin;
    ledc_channel_t ledcChannel;
    uint32_t barrierTimeoutMs;
    uint32_t validationTimeMs;
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
 */
class ExitGateController {
public:
    /**
     * @brief Construct exit gate controller (production mode)
     * Creates own Gate hardware internally
     * @param eventBus Event bus for publishing/subscribing
     * @param ticketService Ticket service
     * @param config Configuration with GPIO pins and timings
     */
    ExitGateController(
        IEventBus& eventBus,
        ITicketService& ticketService,
        const ExitGateConfig& config
    );

    /**
     * @brief Construct exit gate controller (test mode)
     * Uses injected dependencies for testing
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
        uint32_t validationTimeMs = 500
    );

    ~ExitGateController();

    /**
     * @brief Setup GPIO interrupts (only for production mode)
     * Must be called after construction in production mode
     */
    void setupGpioInterrupts();

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
    IGate* m_gate;  // Pointer for optional ownership
    ITicketService& m_ticketService;

    std::unique_ptr<Gate> m_ownedGate;  // Only for production constructor

    ExitGateState m_state;
    uint32_t m_barrierTimeoutMs;
    uint32_t m_validationTimeMs;
    uint32_t m_currentTicketId;
    TimerHandle_t m_barrierTimer;
    TimerHandle_t m_validationTimer;
};
