#ifndef EVENT_DRIVEN_STATE_MACHINE_H
#define EVENT_DRIVEN_STATE_MACHINE_H

#include <functional>
#include <vector>
#include <any>
#include <optional>

// --- Event Type Enums ---

enum class InputEventType {
    Start,
    Stop,
    SpeedUp,
    Reverse,
    Reset
};

enum class OutputEventType {
    MotorOn,
    MotorOff,
    MotorSpeedChange,
    MotorDirectionChange,
    SystemReset
};

// --- Event Structures ---

struct InputEvent {
    InputEventType type;
    std::any payload;

    template <typename T>
    InputEvent(InputEventType t, T p)
        : type(t)
        , payload(std::make_any<T>(std::move(p))) {}

    explicit InputEvent(InputEventType t)
        : type(t) {}

    template <typename T>
    std::optional<T> getPayload() const {
        try {
            return std::any_cast<T>(payload);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
};

struct OutputEvent {
    OutputEventType type;
    std::any payload;

    template <typename T>
    OutputEvent(OutputEventType t, T p)
        : type(t)
        , payload(std::make_any<T>(std::move(p))) {}

    explicit OutputEvent(OutputEventType t)
        : type(t) {}

    template <typename T>
    std::optional<T> getPayload() const {
        try {
            return std::any_cast<T>(payload);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
};

// --- Motor Data Structures ---

struct MotorSpeed {
    int rpm;
};

struct MotorConfig {
    int speed;
    bool direction; // true = forward, false = reverse
};

// --- Event-Driven State Machine ---

class EventDrivenStateMachine {
  public:
    enum class State {
        Idle,
        MotorRunning,
        Stopped
    };

    using EventEmitter = std::function<void(const OutputEvent&)>;

    EventDrivenStateMachine();

    void subscribe(EventEmitter emitter);
    void processEvent(const InputEvent& event);
    State getCurrentState() const { return currentState; }

  private:
    State currentState;
    std::vector<EventEmitter> subscribers;

    template <typename T>
    void emit(OutputEventType type, T&& payload) {
        OutputEvent event(type, std::forward<T>(payload));
        for (auto& subscriber : subscribers) {
            subscriber(event);
        }
    }

    void emit(OutputEventType type);

    void handleIdle(const InputEvent& event);
    void handleMotorRunning(const InputEvent& event);
    void handleStopped(const InputEvent& event);
};

#endif // EVENT_DRIVEN_STATE_MACHINE_H
