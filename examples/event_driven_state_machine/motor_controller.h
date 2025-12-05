#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "event_driven_state_machine.h"
#include <iostream>

// Motor Controller - receives events and controls actual hardware
class MotorController {
public:
    void handleEvent(const OutputEvent& event);

    // Accessors for testing
    bool isMotorRunning() const { return motorState; }
    int getCurrentSpeed() const { return currentSpeed; }

private:
    bool motorState = false;
    int currentSpeed = 0;
};

// Event Logger - demonstrates multiple subscribers
class EventLogger {
public:
    void handleEvent(const OutputEvent& event);
};

#endif // MOTOR_CONTROLLER_H
