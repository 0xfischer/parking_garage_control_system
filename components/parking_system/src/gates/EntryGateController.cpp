#include "EntryGateController.h"
#include "esp_log.h"

static const char* TAG = "EntryGateController";

// Helper function to convert state to string (avoids stack-heavy lambda)
static const char* entryGateStateToString(EntryGateState state) {
    switch (state) {
        case EntryGateState::Idle: return "Idle";
        case EntryGateState::CheckingCapacity: return "CheckingCapacity";
        case EntryGateState::IssuingTicket: return "IssuingTicket";
        case EntryGateState::OpeningBarrier: return "OpeningBarrier";
        case EntryGateState::WaitingForCar: return "WaitingForCar";
        case EntryGateState::CarPassing: return "CarPassing";
        case EntryGateState::WaitingBeforeClose: return "WaitingBeforeClose";
        case EntryGateState::ClosingBarrier: return "ClosingBarrier";
        default: return "Unknown";
    }
}

EntryGateController::EntryGateController(
    IEventBus& eventBus,
    IGpioInput& button,
    IGpioInput& lightBarrier,
    IGpioOutput& motor,
    ITicketService& ticketService,
    uint32_t barrierTimeoutMs
)
    : m_eventBus(eventBus)
    , m_button(button)
    , m_lightBarrier(lightBarrier)
    , m_motor(motor)
    , m_ticketService(ticketService)
    , m_state(EntryGateState::Idle)
    , m_barrierTimeoutMs(barrierTimeoutMs)
    , m_currentTicketId(0)
    , m_barrierTimer(nullptr)
{
    // Subscribe to events
    m_eventBus.subscribe(EventType::EntryButtonPressed,
        [this](const Event& e) { onButtonPressed(e); });
    m_eventBus.subscribe(EventType::EntryLightBarrierBlocked,
        [this](const Event& e) { onLightBarrierBlocked(e); });
    m_eventBus.subscribe(EventType::EntryLightBarrierCleared,
        [this](const Event& e) { onLightBarrierCleared(e); });

    // Create barrier timer
    m_barrierTimer = xTimerCreate(
        "EntryBarrierTimer",
        pdMS_TO_TICKS(m_barrierTimeoutMs),
        pdFALSE,  // One-shot timer
        this,
        barrierTimerCallback
    );

    ESP_LOGI(TAG, "EntryGateController initialized");
}

EntryGateController::~EntryGateController() {
    if (m_barrierTimer) {
        xTimerDelete(m_barrierTimer, 0);
    }
}

const char* EntryGateController::getStateString() const {
    switch (m_state) {
        case EntryGateState::Idle: return "Idle";
        case EntryGateState::CheckingCapacity: return "CheckingCapacity";
        case EntryGateState::IssuingTicket: return "IssuingTicket";
        case EntryGateState::OpeningBarrier: return "OpeningBarrier";
        case EntryGateState::WaitingForCar: return "WaitingForCar";
        case EntryGateState::CarPassing: return "CarPassing";
        case EntryGateState::WaitingBeforeClose: return "WaitingBeforeClose";
        case EntryGateState::ClosingBarrier: return "ClosingBarrier";
        default: return "Unknown";
    }
}

void EntryGateController::setState(EntryGateState newState) {
    if (m_state != newState) {
        ESP_LOGI(TAG, "State: %s -> %s", getStateString(), entryGateStateToString(newState));
        m_state = newState;
    }
}

void EntryGateController::onButtonPressed(const Event& event) {
    (void)event;
    if (m_state != EntryGateState::Idle) {
        ESP_LOGW(TAG, "Button pressed in non-Idle state, ignoring");
        return;
    }

    ESP_LOGI(TAG, "Entry button pressed");
    setState(EntryGateState::CheckingCapacity);

    // Check capacity
    uint32_t activeCount = m_ticketService.getActiveTicketCount();
    uint32_t capacity = m_ticketService.getCapacity();

    if (activeCount >= capacity) {
    ESP_LOGW(TAG, "Parking full! (%lu/%lu)", (unsigned long)activeCount, (unsigned long)capacity);
        m_eventBus.publish(Event(EventType::CapacityFull));
        setState(EntryGateState::Idle);
        return;
    }

    // Capacity available, issue ticket
    setState(EntryGateState::IssuingTicket);
    m_currentTicketId = m_ticketService.getNewTicket();

    if (m_currentTicketId == 0) {
        ESP_LOGE(TAG, "Failed to issue ticket");
        setState(EntryGateState::Idle);
        return;
    }

    ESP_LOGI(TAG, "Ticket issued: ID=%lu", (unsigned long)m_currentTicketId);
    m_eventBus.publish(Event(EventType::TicketIssued, 0, m_currentTicketId));

    // Open barrier
    setState(EntryGateState::OpeningBarrier);
    m_motor.setLevel(true);  // Motor HIGH = opening
    m_eventBus.publish(Event(EventType::EntryBarrierOpened));
    startBarrierTimer();
}

void EntryGateController::onLightBarrierBlocked(const Event& event) {
    (void)event;
    if (m_state == EntryGateState::WaitingForCar) {
        ESP_LOGI(TAG, "Car entering");
        setState(EntryGateState::CarPassing);
    }
}

void EntryGateController::onLightBarrierCleared(const Event& event) {
    (void)event;
    if (m_state == EntryGateState::CarPassing) {
        ESP_LOGI(TAG, "Car passed through, waiting 2 seconds before closing barrier");
        m_eventBus.publish(Event(EventType::CarEnteredParking, 0, m_currentTicketId));

        // Wait 2 seconds before closing barrier
        setState(EntryGateState::WaitingBeforeClose);

        // Start timer with 2 seconds delay
        if (m_barrierTimer) {
            xTimerChangePeriod(m_barrierTimer, pdMS_TO_TICKS(2000), 0);
            xTimerReset(m_barrierTimer, 0);
        }
    }
}

void EntryGateController::onBarrierTimeout() {
    ESP_LOGD(TAG, "Barrier timeout in state: %s", getStateString());

    if (m_state == EntryGateState::OpeningBarrier) {
        // Barrier finished opening
        setState(EntryGateState::WaitingForCar);
    } else if (m_state == EntryGateState::WaitingBeforeClose) {
        // Wait period finished, now close barrier
        ESP_LOGI(TAG, "Wait period finished, closing barrier");
        setState(EntryGateState::ClosingBarrier);
        m_motor.setLevel(false);  // Motor LOW = closing
        m_eventBus.publish(Event(EventType::EntryBarrierClosed));

        // Start timer with normal barrier timeout
        if (m_barrierTimer) {
            xTimerChangePeriod(m_barrierTimer, pdMS_TO_TICKS(m_barrierTimeoutMs), 0);
            xTimerReset(m_barrierTimer, 0);
        }
    } else if (m_state == EntryGateState::ClosingBarrier) {
        // Barrier finished closing
        setState(EntryGateState::Idle);
        m_currentTicketId = 0;
    }
}

void EntryGateController::startBarrierTimer() {
    if (m_barrierTimer) {
        xTimerReset(m_barrierTimer, 0);
    }
}

void EntryGateController::stopBarrierTimer() {
    if (m_barrierTimer) {
        xTimerStop(m_barrierTimer, 0);
    }
}

void EntryGateController::barrierTimerCallback(TimerHandle_t xTimer) {
    auto* controller = static_cast<EntryGateController*>(pvTimerGetTimerID(xTimer));
    if (controller) {
        controller->onBarrierTimeout();
    }
}
