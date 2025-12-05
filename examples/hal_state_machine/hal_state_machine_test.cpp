#include "hal_state_machine.h"
#include <iostream>
#include <cassert>
#include <vector>

// --- Mock GPIO for Testing ---

class MockGpio : public IGpioOutput {
public:
    void setLevel(bool level) override {
        levelHistory.push_back(level);
        currentLevel = level;
    }

    bool getLevel() const override {
        return currentLevel;
    }

    // Test helpers
    const std::vector<bool>& getLevelHistory() const {
        return levelHistory;
    }

    void clearHistory() {
        levelHistory.clear();
    }

private:
    bool currentLevel = false;
    std::vector<bool> levelHistory;
};

// --- Test Cases ---

void test_button_press_triggers_motor_on() {
    std::cout << "TEST: Button press triggers motor ON\n";

    MockGpio motor;
    GateController gate(motor);

    // Initial state
    assert(gate.getCurrentState() == GateController::State::Closed);
    assert(motor.getLevelHistory().empty());

    // Send ButtonPressed event
    gate.handleEvent({EventType::ButtonPressed});

    // Verify state transition and motor activation
    assert(gate.getCurrentState() == GateController::State::Opening);
    assert(motor.getLevelHistory().size() == 1);
    assert(motor.getLevelHistory()[0] == true); // Motor ON
    assert(motor.getLevel() == true);

    std::cout << "  ✓ PASSED\n\n";
}

void test_limit_switch_stops_motor() {
    std::cout << "TEST: Limit switch stops motor\n";

    MockGpio motor;
    GateController gate(motor);

    // First: Open the gate
    gate.handleEvent({EventType::ButtonPressed});
    motor.clearHistory();

    // Send LimitSwitchReached event
    gate.handleEvent({EventType::LimitSwitchReached});

    // Verify motor stopped and state changed to Open
    assert(gate.getCurrentState() == GateController::State::Open);
    assert(motor.getLevelHistory().size() == 1);
    assert(motor.getLevelHistory()[0] == false); // Motor OFF
    assert(motor.getLevel() == false);

    std::cout << "  ✓ PASSED\n\n";
}

void test_full_open_cycle() {
    std::cout << "TEST: Full open cycle (Closed -> Opening -> Open)\n";

    MockGpio motor;
    GateController gate(motor);

    // Step 1: Closed
    assert(gate.getCurrentState() == GateController::State::Closed);

    // Step 2: Button pressed -> Opening
    gate.handleEvent({EventType::ButtonPressed});
    assert(gate.getCurrentState() == GateController::State::Opening);
    assert(motor.getLevel() == true);

    // Step 3: Limit switch reached -> Open
    gate.handleEvent({EventType::LimitSwitchReached});
    assert(gate.getCurrentState() == GateController::State::Open);
    assert(motor.getLevel() == false);

    // Verify complete motor history
    assert(motor.getLevelHistory().size() == 2);
    assert(motor.getLevelHistory()[0] == true);  // Motor ON
    assert(motor.getLevelHistory()[1] == false); // Motor OFF

    std::cout << "  ✓ PASSED\n\n";
}

void test_invalid_events_ignored() {
    std::cout << "TEST: Invalid events are ignored\n";

    MockGpio motor;
    GateController gate(motor);

    // Try to send LimitSwitchReached while closed (invalid)
    gate.handleEvent({EventType::LimitSwitchReached});

    // State should remain Closed
    assert(gate.getCurrentState() == GateController::State::Closed);
    assert(motor.getLevelHistory().empty());

    std::cout << "  ✓ PASSED\n\n";
}

void test_motor_state_tracking() {
    std::cout << "TEST: Motor state tracking through transitions\n";

    MockGpio motor;
    GateController gate(motor);

    // Initial: Motor OFF
    assert(motor.getLevel() == false);

    // Start opening: Motor ON
    gate.handleEvent({EventType::ButtonPressed});
    assert(motor.getLevel() == true);

    // Finish opening: Motor OFF
    gate.handleEvent({EventType::LimitSwitchReached});
    assert(motor.getLevel() == false);

    std::cout << "  ✓ PASSED\n\n";
}

// --- Main Test Runner ---

int main() {
    std::cout << "========================================\n";
    std::cout << "HAL STATE MACHINE TESTS\n";
    std::cout << "========================================\n\n";

    try {
        test_button_press_triggers_motor_on();
        test_limit_switch_stops_motor();
        test_full_open_cycle();
        test_invalid_events_ignored();
        test_motor_state_tracking();

        std::cout << "========================================\n";
        std::cout << "ALL TESTS PASSED ✓\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
