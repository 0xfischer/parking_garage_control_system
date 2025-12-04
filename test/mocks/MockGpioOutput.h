#pragma once

#include "IGpioOutput.h"

/**
 * @brief Mock GPIO output for testing
 *
 * Stores output level for verification in tests.
 */
class MockGpioOutput : public IGpioOutput {
public:
    MockGpioOutput() : m_level(false) {}

    void setLevel(bool high) override {
        m_level = high;
    }

    [[nodiscard]] bool getLevel() const override {
        return m_level;
    }

private:
    bool m_level;
};
