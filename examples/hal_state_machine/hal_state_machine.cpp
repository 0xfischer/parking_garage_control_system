#include "hal_state_machine.h"
#include <iostream>

// --- GateController Implementation ---

GateController::GateController(IGpioOutput& motorGpio)
    : motor(motorGpio), currentState(State::Closed) {}

void GateController::handleEvent(const Event& event) {
    switch (currentState) {
        case State::Closed:
            if (event.type == EventType::ButtonPressed) {
                std::cout << "[GateController] Event: ButtonPressed -> Opening Barrier\n";
                motor.setLevel(true);
                currentState = State::Opening;
            }
            break;

        case State::Opening:
            if (event.type == EventType::LimitSwitchReached) {
                std::cout << "[GateController] Event: LimitSwitchReached -> Barrier Open\n";
                motor.setLevel(false);
                currentState = State::Open;
            }
            break;

        case State::Open:
            // ... logic for closing ...
            break;
    }
}
