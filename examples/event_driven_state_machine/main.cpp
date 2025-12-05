#include "event_driven_state_machine.h"
#include "motor_controller.h"
#include <iostream>

int main() {
    std::cout << "========================================\n";
    std::cout << "EVENT-DRIVEN STATE MACHINE EXAMPLE\n";
    std::cout << "========================================\n\n";

    EventDrivenStateMachine stateMachine;
    MotorController motorController;
    EventLogger logger;

    // Subscribe motor controller to state machine events
    stateMachine.subscribe([&motorController](const OutputEvent& event) {
        motorController.handleEvent(event);
    });

    // Subscribe logger to state machine events
    stateMachine.subscribe([&logger](const OutputEvent& event) {
        logger.handleEvent(event);
    });

    // Send input events to state machine
    std::cout << "=== Sending 'Start' event ===\n";
    stateMachine.processEvent(InputEvent{InputEventType::Start});

    std::cout << "\n=== Sending 'SpeedUp' event ===\n";
    stateMachine.processEvent(InputEvent{InputEventType::SpeedUp});

    std::cout << "\n=== Sending 'Reverse' event ===\n";
    stateMachine.processEvent(InputEvent{InputEventType::Reverse});

    std::cout << "\n=== Sending 'Stop' event ===\n";
    stateMachine.processEvent(InputEvent{InputEventType::Stop});

    std::cout << "\n=== Sending 'Reset' event ===\n";
    stateMachine.processEvent(InputEvent{InputEventType::Reset});

    std::cout << "\n========================================\n";
    std::cout << "EXAMPLE COMPLETED\n";
    std::cout << "========================================\n";

    return 0;
}
