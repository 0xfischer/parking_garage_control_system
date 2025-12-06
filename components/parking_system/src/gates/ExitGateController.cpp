#include "ExitGateController.h"
#include "esp_log.h"

static const char* TAG = "ExitGateController";
static const char *exitGateStateToString(ExitGateState state);

ExitGateController::ExitGateController(
    IEventBus& eventBus,
    IGate& gate,
    ITicketService& ticketService,
    uint32_t barrierTimeoutMs,
    uint32_t validationTimeMs
)
    : m_eventBus(eventBus)
    , m_gate(&gate)
    , m_ticketService(ticketService)
    , m_state(ExitGateState::Idle)
    , m_barrierTimeoutMs(barrierTimeoutMs)
    , m_validationTimeMs(validationTimeMs)
    , m_currentTicketId(0)
    , m_barrierTimer(nullptr)
    , m_validationTimer(nullptr)
{
    // Subscribe to events
    m_eventBus.subscribe(EventType::ExitLightBarrierBlocked,
        [this](const Event& e) { onLightBarrierBlocked(e); });
    m_eventBus.subscribe(EventType::ExitLightBarrierCleared,
        [this](const Event& e) { onLightBarrierCleared(e); });

    // Create timers
    m_barrierTimer = xTimerCreate(
        "ExitBarrierTimer",
        pdMS_TO_TICKS(m_barrierTimeoutMs),
        pdFALSE,
        this,
        barrierTimerCallback
    );

    m_validationTimer = xTimerCreate(
        "ExitValidationTimer",
        pdMS_TO_TICKS(m_validationTimeMs),
        pdFALSE,
        this,
        validationTimerCallback
    );

    ESP_LOGI(TAG, "ExitGateController initialized");
}

ExitGateController::~ExitGateController() {
    if (m_barrierTimer) {
        xTimerDelete(m_barrierTimer, 0);
    }
    if (m_validationTimer) {
        xTimerDelete(m_validationTimer, 0);
    }
}

void ExitGateController::setupGpioInterrupts() {
    ESP_LOGI(TAG, "Exit gate GPIO interrupts configured");
}

const char* ExitGateController::getStateString() const {
    switch (m_state) {
        case ExitGateState::Idle: return "Idle";
        case ExitGateState::ValidatingTicket: return "ValidatingTicket";
        case ExitGateState::OpeningBarrier: return "OpeningBarrier";
        case ExitGateState::WaitingForCarToPass: return "WaitingForCarToPass";
        case ExitGateState::CarPassing: return "CarPassing";
        case ExitGateState::WaitingBeforeClose: return "WaitingBeforeClose";
        case ExitGateState::ClosingBarrier: return "ClosingBarrier";
        default: return "Unknown";
    }
}

void ExitGateController::setState(ExitGateState newState) {
    if (m_state != newState) {
        ESP_LOGI(TAG, "State: %s -> %s", getStateString(), exitGateStateToString(newState));
        m_state = newState;
    }
}

void ExitGateController::onLightBarrierBlocked(const Event& event) {
    (void)event;
    if (m_state == ExitGateState::WaitingForCarToPass) {
        ESP_LOGI(TAG, "Car entering exit barrier");
        setState(ExitGateState::CarPassing);
    }
    // Note: Idle state does not react to light barrier
    // Exit sequence must be started manually via ticket_validate command
}

void ExitGateController::onLightBarrierCleared(const Event& event) {
    (void)event;
    if (m_state == ExitGateState::CarPassing) {
        ESP_LOGI(TAG, "Car exited parking, waiting 2 seconds before closing barrier");
        m_eventBus.publish(Event(EventType::CarExitedParking, 0, m_currentTicketId));

        // Wait 2 seconds before closing barrier
        setState(ExitGateState::WaitingBeforeClose);

        // Start timer with 2 seconds delay
        if (m_barrierTimer) {
            xTimerChangePeriod(m_barrierTimer, pdMS_TO_TICKS(2000), 0);
            xTimerReset(m_barrierTimer, 0);
        }
    }
}

void ExitGateController::onValidationTimeout() {
    // This function is not used in manual validation mode
    // Validation must be triggered manually via validateTicketManually()
    ESP_LOGW(TAG, "Validation timeout - should not happen in manual mode");
}

bool ExitGateController::validateTicketManually(uint32_t ticketId) {
    if (m_state != ExitGateState::Idle) {
        ESP_LOGW(TAG, "Cannot validate manually - must be in Idle state (current: %s)", getStateString());
        return false;
    }

    ESP_LOGI(TAG, "Starting manual ticket validation for ID=%lu", (unsigned long)ticketId);
    setState(ExitGateState::ValidatingTicket);
    m_currentTicketId = ticketId;

    // Check if ticket is paid
    Ticket ticket;
    if (m_ticketService.getTicketInfo(ticketId, ticket)) {
        if (!ticket.isPaid) {
            ESP_LOGW(TAG, "Ticket not paid: ID=%lu - use 'ticket_pay %lu' command first!",
                     (unsigned long)ticketId, (unsigned long)ticketId);
            m_eventBus.publish(Event(EventType::TicketRejected));
            setState(ExitGateState::Idle);
            return false;
        }

        if (m_ticketService.validateAndUseTicket(ticketId)) {
            ESP_LOGI(TAG, "Ticket validation successful: ID=%lu", (unsigned long)ticketId);
            m_eventBus.publish(Event(EventType::TicketValidated, 0, ticketId));

            setState(ExitGateState::OpeningBarrier);
            m_gate->open();
            m_eventBus.publish(Event(EventType::ExitBarrierOpened));
            startBarrierTimer();
            return true;
        }
    }

    ESP_LOGW(TAG, "Ticket validation failed: ID=%lu", (unsigned long)ticketId);
    m_eventBus.publish(Event(EventType::TicketRejected));
    setState(ExitGateState::Idle);
    return false;
}

void ExitGateController::onBarrierTimeout() {
    ESP_LOGD(TAG, "Barrier timeout in state: %s", getStateString());

    if (m_state == ExitGateState::OpeningBarrier) {
        setState(ExitGateState::WaitingForCarToPass);
    } else if (m_state == ExitGateState::WaitingBeforeClose) {
        // Wait period finished, now close barrier
        ESP_LOGI(TAG, "Wait period finished, closing barrier");
        setState(ExitGateState::ClosingBarrier);
        m_gate->close();
        m_eventBus.publish(Event(EventType::ExitBarrierClosed));

        // Start timer with normal barrier timeout
        if (m_barrierTimer) {
            xTimerChangePeriod(m_barrierTimer, pdMS_TO_TICKS(m_barrierTimeoutMs), 0);
            xTimerReset(m_barrierTimer, 0);
        }
    } else if (m_state == ExitGateState::ClosingBarrier) {
        setState(ExitGateState::Idle);
        m_currentTicketId = 0;
    }
}

void ExitGateController::startBarrierTimer() {
    if (m_barrierTimer) {
        xTimerReset(m_barrierTimer, 0);
    }
}

void ExitGateController::stopBarrierTimer() {
    if (m_barrierTimer) {
        xTimerStop(m_barrierTimer, 0);
    }
}

void ExitGateController::startValidationTimer() {
    if (m_validationTimer) {
        xTimerReset(m_validationTimer, 0);
    }
}

void ExitGateController::stopValidationTimer() {
    if (m_validationTimer) {
        xTimerStop(m_validationTimer, 0);
    }
}

void ExitGateController::barrierTimerCallback(TimerHandle_t xTimer) {
    auto* controller = static_cast<ExitGateController*>(pvTimerGetTimerID(xTimer));
    if (controller) {
        controller->onBarrierTimeout();
    }
}

void ExitGateController::validationTimerCallback(TimerHandle_t xTimer) {
    auto* controller = static_cast<ExitGateController*>(pvTimerGetTimerID(xTimer));
    if (controller) {
        controller->onValidationTimeout();
    }
}

// Helper function to convert state to string (avoids stack-heavy lambda)
static const char *exitGateStateToString(ExitGateState state)
{
    switch (state)
    {
    case ExitGateState::Idle:
        return "Idle";
    case ExitGateState::ValidatingTicket:
        return "ValidatingTicket";
    case ExitGateState::OpeningBarrier:
        return "OpeningBarrier";
    case ExitGateState::WaitingForCarToPass:
        return "WaitingForCarToPass";
    case ExitGateState::CarPassing:
        return "CarPassing";
    case ExitGateState::WaitingBeforeClose:
        return "WaitingBeforeClose";
    case ExitGateState::ClosingBarrier:
        return "ClosingBarrier";
    default:
        return "Unknown";
    }
}
