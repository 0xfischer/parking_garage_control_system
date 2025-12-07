#ifndef HAL_STATE_MACHINE_H
#define HAL_STATE_MACHINE_H

// --- 1. Hardware Abstraction Layer (HAL) ---

// Abstract Interface for GPIO Output
class IGpioOutput {
  public:
    virtual void setLevel(bool level) = 0;
    virtual bool getLevel() const = 0;
    virtual ~IGpioOutput() = default;
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
    enum class State { Closed,
                       Opening,
                       Open };

    // Dependency Injection: Logic depends on Interface, not concrete Hardware
    explicit GateController(IGpioOutput& motorGpio);

    void handleEvent(const Event& event);
    State getCurrentState() const { return currentState; }

  private:
    IGpioOutput& motor; // Reference to abstract hardware
    State currentState;
};

#endif // HAL_STATE_MACHINE_H
