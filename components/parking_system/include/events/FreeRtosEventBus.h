#pragma once

#include "IEventBus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <map>
#include <vector>

/**
 * @brief FreeRTOS implementation of event bus
 *
 * Thread-safe event bus using FreeRTOS queue and mutex.
 * Supports both asynchronous event publishing and synchronous event processing.
 * Can run its own event loop task for automatic event dispatching.
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

    /**
     * @brief Start the internal event loop task
     *
     * Creates a FreeRTOS task that continuously processes events from the queue.
     * This is the recommended way to run the event bus in production.
     *
     * @param stackSize Task stack size in bytes (default: 4096)
     * @param priority Task priority (default: 5)
     * @param taskName Name for the task (default: "event_loop")
     */
    void startEventLoop(uint32_t stackSize = 4096, UBaseType_t priority = 5,
                        const char* taskName = "event_loop");

    /**
     * @brief Stop the internal event loop task
     *
     * Stops and deletes the event loop task if running.
     */
    void stopEventLoop();

    /**
     * @brief Check if event loop is running
     * @return true if event loop task is active
     */
    [[nodiscard]] bool isEventLoopRunning() const;

  private:
    void dispatchEvent(const Event& event);
    static void eventLoopTask(void* pvParameters);

    QueueHandle_t m_queue;
    SemaphoreHandle_t m_mutex;
    std::map<EventType, std::vector<std::function<void(const Event&)>>> m_subscribers;
    TaskHandle_t m_eventLoopTask = nullptr;
    volatile bool m_stopRequested = false;
};
