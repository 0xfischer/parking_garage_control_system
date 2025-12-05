#include "motor_controller.h"

void MotorController::handleEvent(const OutputEvent& event) {
    switch (event.type) {
        case OutputEventType::MotorOn:
            if (auto config = event.getPayload<MotorConfig>()) {
                std::cout << "[MotorController] Motor turned ON: "
                          << config->speed << " RPM, "
                          << (config->direction ? "Forward" : "Reverse") << "\n";
                motorState = true;
                currentSpeed = config->speed;
            }
            break;
        case OutputEventType::MotorOff:
            std::cout << "[MotorController] Motor turned OFF\n";
            motorState = false;
            currentSpeed = 0;
            break;
        case OutputEventType::MotorSpeedChange:
            if (auto speed = event.getPayload<MotorSpeed>()) {
                std::cout << "[MotorController] Speed changed to: "
                          << speed->rpm << " RPM\n";
                currentSpeed = speed->rpm;
            }
            break;
        case OutputEventType::MotorDirectionChange:
            if (auto config = event.getPayload<MotorConfig>()) {
                std::cout << "[MotorController] Direction changed: "
                          << config->speed << " RPM, "
                          << (config->direction ? "Forward" : "Reverse") << "\n";
                currentSpeed = config->speed;
            }
            break;
        case OutputEventType::SystemReset:
            std::cout << "[MotorController] System reset\n";
            motorState = false;
            currentSpeed = 0;
            break;
    }
}

void EventLogger::handleEvent(const OutputEvent& event) {
    std::cout << "[EventLogger] Event: ";

    switch (event.type) {
        case OutputEventType::MotorOn:              std::cout << "MotorOn"; break;
        case OutputEventType::MotorOff:             std::cout << "MotorOff"; break;
        case OutputEventType::MotorSpeedChange:     std::cout << "MotorSpeedChange"; break;
        case OutputEventType::MotorDirectionChange: std::cout << "MotorDirectionChange"; break;
        case OutputEventType::SystemReset:          std::cout << "SystemReset"; break;
    }

    // Try to extract known payload types
    if (auto config = event.getPayload<MotorConfig>()) {
        std::cout << " | Payload: " << config->speed << " RPM, "
                  << (config->direction ? "Forward" : "Reverse");
    } else if (auto speed = event.getPayload<MotorSpeed>()) {
        std::cout << " | Payload: " << speed->rpm << " RPM";
    } else if (event.payload.has_value()) {
        std::cout << " | Payload: <unknown type>";
    }

    std::cout << "\n";
}
