#pragma once

#include "IGpioInput.h"

/**
 * @brief Mock GPIO input for testing
 *
 * Allows simulation of GPIO state changes and interrupt triggering.
 */
class MockGpioInput : public IGpioInput {
  public:
    MockGpioInput()
        : m_level(false)
        , m_handler(nullptr)
        , m_interruptEnabled(false) {}

    [[nodiscard]] bool getLevel() const override {
        return m_level;
    }

    void setInterruptHandler(std::function<void(bool level)> handler) override {
        m_handler = std::move(handler);
    }

    void enableInterrupt() override {
        m_interruptEnabled = true;
    }

    void disableInterrupt() override {
        m_interruptEnabled = false;
    }

    // Test helpers
    void setLevel(bool level) {
        m_level = level;
    }

    void simulateInterrupt(bool level) {
        m_level = level;
        if (m_interruptEnabled && m_handler) {
            m_handler(level);
        }
    }

    [[nodiscard]] bool isInterruptEnabled() const {
        return m_interruptEnabled;
    }

  private:
    bool m_level;
    std::function<void(bool)> m_handler;
    bool m_interruptEnabled;
};
