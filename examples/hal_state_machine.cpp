#include <iostream>
#include <memory>

// --- 1. Hardware Abstraction Layer (HAL) ---

// Abstract Interface for GPIO Output
class IGpioOutput {
public:
    virtual void setLevel(bool level) = 0;
    virtual ~IGpioOutput() = default;
};

// Concrete Implementation (e.g., for ESP32 or Simulation)
class Esp32Gpio : public IGpioOutput {
public:
    void setLevel(bool level) override {
        // In reality: gpio_set_level(PIN, level);
        std::cout << "[HAL] GPIO set to " << (level ? "HIGH" : "LOW") << "\n";
    }
};

// --- 2. Event System ---

enum class EventType {
    ButtonPressed,
    LimitSwitchReached
};

struct Event {
    EventType type;
};

// --- 3. Logic / State Machine ---

class GateController {
public:
    enum class State { Closed, Opening, Open };

    // Dependency Injection: Logic depends on Interface, not concrete Hardware
    GateController(IGpioOutput& motorGpio) 
        : motor(motorGpio), currentState(State::Closed) {}

    void handleEvent(const Event& event) {
        switch (currentState) {
            case State::Closed:
                if (event.type == EventType::ButtonPressed) {
                    std::cout << "Event: ButtonPressed -> Opening Barrier\n";
                    // Action: Control Hardware via Abstraction
                    motor.setLevel(true); 
                    currentState = State::Opening;
                }
                break;

            case State::Opening:
                if (event.type == EventType::LimitSwitchReached) {
                    std::cout << "Event: LimitSwitchReached -> Barrier Open\n";
                    motor.setLevel(false);
                    currentState = State::Open;
                }
                break;

            case State::Open:
                // ... logic for closing ...
                break;
        }
    }

private:
    IGpioOutput& motor; // Reference to abstract hardware
    State currentState;
};

int main() {
    // Setup
    Esp32Gpio realMotorGpio;
    GateController gate(realMotorGpio);

    // Event Loop Simulation
    gate.handleEvent({EventType::ButtonPressed});      // Triggers Closed -> Opening
    gate.handleEvent({EventType::LimitSwitchReached}); // Triggers Opening -> Open

    return 0;
}
