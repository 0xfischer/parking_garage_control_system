#pragma once

#include "IGate.h"

/**
 * @brief Mock implementation of IGate for testing
 */
class MockGate : public IGate {
public:
    MockGate() : m_isOpen(false), m_carDetected(false) {}

    void open() override {
        m_isOpen = true;
    }

    void close() override {
        m_isOpen = false;
    }

    bool isOpen() const override {
        return m_isOpen;
    }

    bool isCarDetected() const override {
        return m_carDetected;
    }

    // Test helpers
    void setCarDetected(bool detected) {
        m_carDetected = detected;
    }

private:
    bool m_isOpen;
    bool m_carDetected;
};
