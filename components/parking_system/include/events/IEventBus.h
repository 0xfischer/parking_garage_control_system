#pragma once

#include "Event.h"
#include <functional>

/**
 * @brief Interface for event bus
 *
 * Provides publish-subscribe mechanism for event-driven architecture.
 * Thread-safe implementation required for FreeRTOS multi-tasking.
 */
class IEventBus {
public:
    virtual ~IEventBus() = default;

    /**
     * @brief Subscribe to specific event type
     * @param type Event type to subscribe to
     * @param handler Callback function called when event occurs
     */
    virtual void subscribe(EventType type, std::function<void(const Event&)> handler) = 0;

    /**
     * @brief Publish event to all subscribers
     * @param event Event to publish
     */
    virtual void publish(const Event& event) = 0;

    /**
     * @brief Process all pending events (for synchronous testing)
     */
    virtual void processAllPending() = 0;

    /**
     * @brief Wait for next event (blocking, for FreeRTOS tasks)
     * @param outEvent Output event structure
     * @param timeoutMs Timeout in milliseconds (portMAX_DELAY for infinite)
     * @return true if event received, false on timeout
     */
    [[nodiscard]] virtual bool waitForEvent(Event& outEvent, uint32_t timeoutMs) = 0;
};
