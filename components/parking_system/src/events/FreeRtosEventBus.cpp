#include "FreeRtosEventBus.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "FreeRtosEventBus";

FreeRtosEventBus::FreeRtosEventBus(size_t queueSize) {
    m_queue = xQueueCreate(queueSize, sizeof(Event));
    if (!m_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
    }

    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }

    ESP_LOGI(TAG, "EventBus created (queue size: %d)", queueSize);
}

FreeRtosEventBus::~FreeRtosEventBus() {
    // Stop event loop first to prevent access to deleted resources
    stopEventLoop();

    if (m_queue) {
        vQueueDelete(m_queue);
    }
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
    }
}

void FreeRtosEventBus::subscribe(EventType type, std::function<void(const Event&)> handler) {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        m_subscribers[type].push_back(std::move(handler));
        ESP_LOGI(TAG, "Subscriber added for event: %s", eventTypeToString(type));
        xSemaphoreGive(m_mutex);
    }
}

void FreeRtosEventBus::publish(const Event& event) {
    if (!m_queue) {
        ESP_LOGE(TAG, "Cannot publish: queue not initialized");
        return;
    }

    // Add timestamp if not set
    Event timestampedEvent = event;
    if (timestampedEvent.timestamp == 0) {
        timestampedEvent.timestamp = esp_timer_get_time();
    }

    if (xQueueSend(m_queue, &timestampedEvent, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full, dropping event: %s", eventTypeToString(event.type));
    }
}

bool FreeRtosEventBus::publishFromISR(const Event& event) {
    if (!m_queue) {
        return false;
    }

    Event timestampedEvent = event;
    if (timestampedEvent.timestamp == 0) {
        timestampedEvent.timestamp = esp_timer_get_time();
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xQueueSendFromISR(m_queue, &timestampedEvent, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return result == pdTRUE;
}

void FreeRtosEventBus::processAllPending() {
    Event event;
    while (xQueueReceive(m_queue, &event, 0) == pdTRUE) {
        dispatchEvent(event);
    }
}

bool FreeRtosEventBus::waitForEvent(Event& outEvent, uint32_t timeoutMs) {
    if (!m_queue) {
        return false;
    }

    TickType_t ticks = (timeoutMs == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);
    if (xQueueReceive(m_queue, &outEvent, ticks) == pdTRUE) {
        dispatchEvent(outEvent);
        return true;
    }

    return false;
}

void FreeRtosEventBus::dispatchEvent(const Event& event) {
    if (xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = m_subscribers.find(event.type);
        if (it != m_subscribers.end()) {
            ESP_LOGD(TAG, "Dispatching event: %s to %d subscribers",
                     eventTypeToString(event.type), it->second.size());

            for (const auto& handler : it->second) {
                if (handler) {
                    handler(event);
                }
            }
        }
        xSemaphoreGive(m_mutex);
    }
}

void FreeRtosEventBus::startEventLoop(uint32_t stackSize, UBaseType_t priority,
                                       const char* taskName) {
    if (m_eventLoopTask != nullptr) {
        ESP_LOGW(TAG, "Event loop already running");
        return;
    }

    m_stopRequested = false;

    BaseType_t result = xTaskCreate(
        eventLoopTask,
        taskName,
        stackSize,
        this,
        priority,
        &m_eventLoopTask);

    if (result == pdPASS) {
        ESP_LOGI(TAG, "Event loop task started (stack: %lu, priority: %u)",
                 stackSize, priority);
    } else {
        ESP_LOGE(TAG, "Failed to create event loop task");
        m_eventLoopTask = nullptr;
    }
}

void FreeRtosEventBus::stopEventLoop() {
    if (m_eventLoopTask == nullptr) {
        ESP_LOGW(TAG, "Event loop not running");
        return;
    }

    ESP_LOGI(TAG, "Stopping event loop...");
    m_stopRequested = true;

    // Give the task time to exit gracefully
    vTaskDelay(pdMS_TO_TICKS(100));

    // If still running, delete it
    if (m_eventLoopTask != nullptr) {
        vTaskDelete(m_eventLoopTask);
        m_eventLoopTask = nullptr;
    }

    ESP_LOGI(TAG, "Event loop stopped");
}

bool FreeRtosEventBus::isEventLoopRunning() const {
    return m_eventLoopTask != nullptr && !m_stopRequested;
}

void FreeRtosEventBus::eventLoopTask(void* pvParameters) {
    auto* self = static_cast<FreeRtosEventBus*>(pvParameters);

    ESP_LOGI(TAG, "Event loop task running");

    Event event;
    while (!self->m_stopRequested) {
        // Wait for event with timeout to check stop flag periodically
        if (self->waitForEvent(event, 100)) {
            ESP_LOGD(TAG, "Event processed: %s", eventTypeToString(event.type));
        }
    }

    ESP_LOGI(TAG, "Event loop task exiting");
    self->m_eventLoopTask = nullptr;
    vTaskDelete(nullptr);
}
