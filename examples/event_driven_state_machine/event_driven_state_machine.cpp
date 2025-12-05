#include "event_driven_state_machine.h"

EventDrivenStateMachine::EventDrivenStateMachine()
    : currentState(State::Idle) {}

void EventDrivenStateMachine::subscribe(EventEmitter emitter) {
    subscribers.push_back(std::move(emitter));
}

void EventDrivenStateMachine::processEvent(const InputEvent& event) {
    switch (currentState) {
        case State::Idle:
            handleIdle(event);
            break;
        case State::MotorRunning:
            handleMotorRunning(event);
            break;
        case State::Stopped:
            handleStopped(event);
            break;
    }
}

void EventDrivenStateMachine::emit(OutputEventType type) {
    OutputEvent event(type);
    for (auto& subscriber : subscribers) {
        subscriber(event);
    }
}

void EventDrivenStateMachine::handleIdle(const InputEvent& event) {
    switch (event.type) {
        case InputEventType::Start:
            currentState = State::MotorRunning;
            emit(OutputEventType::MotorOn, MotorConfig{100, true});
            break;
        default:
            break;
    }
}

void EventDrivenStateMachine::handleMotorRunning(const InputEvent& event) {
    switch (event.type) {
        case InputEventType::Stop:
            currentState = State::Stopped;
            emit(OutputEventType::MotorOff);
            break;
        case InputEventType::SpeedUp:
            emit(OutputEventType::MotorSpeedChange, MotorSpeed{150});
            break;
        case InputEventType::Reverse:
            emit(OutputEventType::MotorDirectionChange, MotorConfig{100, false});
            break;
        default:
            break;
    }
}

void EventDrivenStateMachine::handleStopped(const InputEvent& event) {
    switch (event.type) {
        case InputEventType::Reset:
            currentState = State::Idle;
            emit(OutputEventType::SystemReset);
            break;
        default:
            break;
    }
}
