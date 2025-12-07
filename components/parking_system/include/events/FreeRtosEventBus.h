#pragma once

#include "IEventBus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <map>
#include <vector>

/**
 * @brief FreeRTOS implementation of event bus
 *
 * Thread-safe event bus using FreeRTOS queue and mutex.
 * Supports both asynchronous event publishing and synchronous event processing.
 */
class FreeRtosEventBus : public IEventBus {
  public:
    /**
     * @brief Construct event bus
     * @param queueSize Maximum number of queued events
     */
    explicit FreeRtosEventBus(size_t queueSize = 32);
    ~FreeRtosEventBus() override;

    // Prevent copying
    FreeRtosEventBus(const FreeRtosEventBus&) = delete;
    FreeRtosEventBus& operator=(const FreeRtosEventBus&) = delete;

    void subscribe(EventType type, std::function<void(const Event&)> handler) override;
    void publish(const Event& event) override;
    void processAllPending() override;
    [[nodiscard]] bool waitForEvent(Event& outEvent, uint32_t timeoutMs) override;

    /**
     * @brief Publish event from ISR context
     * @param event Event to publish
     * @return true if event was queued successfully
     */
    bool publishFromISR(const Event& event);

  private:
    void dispatchEvent(const Event& event);

    QueueHandle_t m_queue;
    SemaphoreHandle_t m_mutex;
    std::map<EventType, std::vector<std::function<void(const Event&)>>> m_subscribers;
};
