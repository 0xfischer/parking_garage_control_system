#include "event_driven_state_machine.h"
#include <iostream>
#include <vector>
#include <cassert>

// --- Mock Motor Controller for Testing ---

class MockMotorController {
public:
    void handleEvent(const OutputEvent& event) {
        receivedEvents.push_back(event.type);

        switch (event.type) {
            case OutputEventType::MotorOn:
                if (auto config = event.getPayload<MotorConfig>()) {
                    motorRunning = true;
                    currentSpeed = config->speed;
                    direction = config->direction;
                }
                break;
            case OutputEventType::MotorOff:
                motorRunning = false;
                currentSpeed = 0;
                break;
            case OutputEventType::MotorSpeedChange:
                if (auto speed = event.getPayload<MotorSpeed>()) {
                    currentSpeed = speed->rpm;
                }
                break;
            case OutputEventType::MotorDirectionChange:
                if (auto config = event.getPayload<MotorConfig>()) {
                    currentSpeed = config->speed;
                    direction = config->direction;
                }
                break;
            case OutputEventType::SystemReset:
                motorRunning = false;
                currentSpeed = 0;
                direction = true;
                break;
        }
    }

    bool isMotorRunning() const { return motorRunning; }
    int getCurrentSpeed() const { return currentSpeed; }
    bool getDirection() const { return direction; }
    const std::vector<OutputEventType>& getReceivedEvents() const { return receivedEvents; }
    void clearEvents() { receivedEvents.clear(); }

private:
    bool motorRunning = false;
    int currentSpeed = 0;
    bool direction = true;
    std::vector<OutputEventType> receivedEvents;
};

// --- Event Recorder for Assertions ---

class EventRecorder {
public:
    void handleEvent(const OutputEvent& event) {
        events.push_back(event);
    }

    size_t getEventCount() const { return events.size(); }

    OutputEventType getEventType(size_t index) const {
        return events.at(index).type;
    }

    template<typename T>
    std::optional<T> getEventPayload(size_t index) const {
        return events.at(index).getPayload<T>();
    }

    void clear() { events.clear(); }

private:
    std::vector<OutputEvent> events;
};

// --- Test Cases ---

void test_idle_to_running_transition() {
    std::cout << "TEST: Idle -> Running transition\n";

    EventDrivenStateMachine sm;
    MockMotorController motor;

    sm.subscribe([&motor](const OutputEvent& event) {
        motor.handleEvent(event);
    });

    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Idle);
    assert(!motor.isMotorRunning());

    sm.processEvent(InputEvent{InputEventType::Start});

    assert(sm.getCurrentState() == EventDrivenStateMachine::State::MotorRunning);
    assert(motor.isMotorRunning());
    assert(motor.getCurrentSpeed() == 100);
    assert(motor.getDirection() == true);
    assert(motor.getReceivedEvents().size() == 1);
    assert(motor.getReceivedEvents()[0] == OutputEventType::MotorOn);

    std::cout << "  ✓ PASSED\n\n";
}

void test_speed_change_while_running() {
    std::cout << "TEST: Speed change while running\n";

    EventDrivenStateMachine sm;
    MockMotorController motor;

    sm.subscribe([&motor](const OutputEvent& event) {
        motor.handleEvent(event);
    });

    sm.processEvent(InputEvent{InputEventType::Start});
    motor.clearEvents();

    sm.processEvent(InputEvent{InputEventType::SpeedUp});

    assert(sm.getCurrentState() == EventDrivenStateMachine::State::MotorRunning);
    assert(motor.isMotorRunning());
    assert(motor.getCurrentSpeed() == 150);
    assert(motor.getReceivedEvents().size() == 1);
    assert(motor.getReceivedEvents()[0] == OutputEventType::MotorSpeedChange);

    std::cout << "  ✓ PASSED\n\n";
}

void test_direction_reversal() {
    std::cout << "TEST: Direction reversal\n";

    EventDrivenStateMachine sm;
    MockMotorController motor;

    sm.subscribe([&motor](const OutputEvent& event) {
        motor.handleEvent(event);
    });

    sm.processEvent(InputEvent{InputEventType::Start});
    assert(motor.getDirection() == true);
    motor.clearEvents();

    sm.processEvent(InputEvent{InputEventType::Reverse});

    assert(motor.getDirection() == false);
    assert(motor.getReceivedEvents().size() == 1);
    assert(motor.getReceivedEvents()[0] == OutputEventType::MotorDirectionChange);

    std::cout << "  ✓ PASSED\n\n";
}

void test_full_lifecycle() {
    std::cout << "TEST: Full lifecycle (Idle -> Running -> Stopped -> Idle)\n";

    EventDrivenStateMachine sm;
    EventRecorder recorder;

    sm.subscribe([&recorder](const OutputEvent& event) {
        recorder.handleEvent(event);
    });

    sm.processEvent(InputEvent{InputEventType::Start});
    assert(sm.getCurrentState() == EventDrivenStateMachine::State::MotorRunning);
    assert(recorder.getEventCount() == 1);
    assert(recorder.getEventType(0) == OutputEventType::MotorOn);

    sm.processEvent(InputEvent{InputEventType::Stop});
    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Stopped);
    assert(recorder.getEventCount() == 2);
    assert(recorder.getEventType(1) == OutputEventType::MotorOff);

    sm.processEvent(InputEvent{InputEventType::Reset});
    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Idle);
    assert(recorder.getEventCount() == 3);
    assert(recorder.getEventType(2) == OutputEventType::SystemReset);

    std::cout << "  ✓ PASSED\n\n";
}

void test_invalid_transitions_are_ignored() {
    std::cout << "TEST: Invalid transitions are ignored\n";

    EventDrivenStateMachine sm;
    MockMotorController motor;

    sm.subscribe([&motor](const OutputEvent& event) {
        motor.handleEvent(event);
    });

    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Idle);
    sm.processEvent(InputEvent{InputEventType::Stop});
    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Idle);
    assert(motor.getReceivedEvents().empty());

    sm.processEvent(InputEvent{InputEventType::Reset});
    assert(sm.getCurrentState() == EventDrivenStateMachine::State::Idle);
    assert(motor.getReceivedEvents().empty());

    std::cout << "  ✓ PASSED\n\n";
}

void test_multiple_subscribers() {
    std::cout << "TEST: Multiple subscribers receive events\n";

    EventDrivenStateMachine sm;
    MockMotorController motor1;
    MockMotorController motor2;
    EventRecorder recorder;

    sm.subscribe([&motor1](const OutputEvent& event) {
        motor1.handleEvent(event);
    });
    sm.subscribe([&motor2](const OutputEvent& event) {
        motor2.handleEvent(event);
    });
    sm.subscribe([&recorder](const OutputEvent& event) {
        recorder.handleEvent(event);
    });

    sm.processEvent(InputEvent{InputEventType::Start});

    assert(motor1.getReceivedEvents().size() == 1);
    assert(motor2.getReceivedEvents().size() == 1);
    assert(recorder.getEventCount() == 1);

    assert(motor1.isMotorRunning());
    assert(motor2.isMotorRunning());

    std::cout << "  ✓ PASSED\n\n";
}

void test_payload_extraction() {
    std::cout << "TEST: Type-safe payload extraction\n";

    EventDrivenStateMachine sm;
    EventRecorder recorder;

    sm.subscribe([&recorder](const OutputEvent& event) {
        recorder.handleEvent(event);
    });

    sm.processEvent(InputEvent{InputEventType::Start});

    auto config = recorder.getEventPayload<MotorConfig>(0);
    assert(config.has_value());
    assert(config->speed == 100);
    assert(config->direction == true);

    sm.processEvent(InputEvent{InputEventType::SpeedUp});
    auto speed = recorder.getEventPayload<MotorSpeed>(1);
    assert(speed.has_value());
    assert(speed->rpm == 150);

    std::cout << "  ✓ PASSED\n\n";
}

// --- Main Test Runner ---

int main() {
    std::cout << "========================================\n";
    std::cout << "EVENT-DRIVEN STATE MACHINE TESTS\n";
    std::cout << "========================================\n\n";

    try {
        test_idle_to_running_transition();
        test_speed_change_while_running();
        test_direction_reversal();
        test_full_lifecycle();
        test_invalid_transitions_are_ignored();
        test_multiple_subscribers();
        test_payload_extraction();

        std::cout << "========================================\n";
        std::cout << "ALL TESTS PASSED ✓\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
