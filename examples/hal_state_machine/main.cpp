#include "hal_state_machine.h"
#include "esp32_gpio.h"
#include <iostream>

int main() {
    std::cout << "========================================\n";
    std::cout << "HAL STATE MACHINE EXAMPLE\n";
    std::cout << "========================================\n\n";

    // Setup: Create hardware abstraction and state machine
    Esp32Gpio realMotorGpio;
    GateController gate(realMotorGpio);

    std::cout << "Initial state: Closed\n\n";

    // Event Loop Simulation
    std::cout << "=== Sending ButtonPressed event ===\n";
    gate.handleEvent({EventType::ButtonPressed}); // Triggers Closed -> Opening
    std::cout << "Current state: "
              << (gate.getCurrentState() == GateController::State::Opening ? "Opening" : "Other")
              << "\n\n";

    std::cout << "=== Sending LimitSwitchReached event ===\n";
    gate.handleEvent({EventType::LimitSwitchReached}); // Triggers Opening -> Open
    std::cout << "Current state: "
              << (gate.getCurrentState() == GateController::State::Open ? "Open" : "Other")
              << "\n\n";

    std::cout << "========================================\n";
    std::cout << "EXAMPLE COMPLETED\n";
    std::cout << "========================================\n";

    return 0;
}
