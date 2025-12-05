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
        portYIELD_FROM_ISR();
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
