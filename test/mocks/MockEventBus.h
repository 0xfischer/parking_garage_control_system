#pragma once

#include "IEventBus.h"
#include <queue>
#include <map>
#include <vector>

/**
 * @brief Mock event bus for synchronous testing
 *
 * Provides deterministic event processing for unit tests.
 */
class MockEventBus : public IEventBus {
public:
    MockEventBus() = default;

    void subscribe(EventType type, std::function<void(const Event&)> handler) override {
        m_subscribers[type].push_back(std::move(handler));
    }

    void publish(const Event& event) override {
        m_queue.push(event);
        m_history.push_back(event);
    }

    void processAllPending() override {
        while (!m_queue.empty()) {
            Event event = m_queue.front();
            m_queue.pop();
            dispatchEvent(event);
        }
    }

    [[nodiscard]] bool waitForEvent(Event& outEvent, uint32_t timeoutMs) override {
        (void)timeoutMs;
        if (m_queue.empty()) {
            return false;
        }

        outEvent = m_queue.front();
        m_queue.pop();
        dispatchEvent(outEvent);
        return true;
    }

    // Test helpers
    [[nodiscard]] size_t getPendingEventCount() const {
        return m_queue.size();
    }

    void clear() {
        while (!m_queue.empty()) {
            m_queue.pop();
        }
        m_history.clear();
    }

    // Inspect published events history
    const std::vector<Event>& history() const { return m_history; }
    size_t historyCount() const { return m_history.size(); }

private:
    void dispatchEvent(const Event& event) {
        auto it = m_subscribers.find(event.type);
        if (it != m_subscribers.end()) {
            for (const auto& handler : it->second) {
                if (handler) {
                    handler(event);
                }
            }
        }
    }

    std::queue<Event> m_queue;
    std::map<EventType, std::vector<std::function<void(const Event&)>>> m_subscribers;
    std::vector<Event> m_history;
};
