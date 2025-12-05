# Event-Driven State Machine Example

Completely event-driven state machine with zero hardware dependencies using modern C++20 features.

## Overview

This example demonstrates a fully decoupled event-driven architecture where the state machine emits events to multiple subscribers without knowing who receives them.

**Architecture Pattern:** Publisher-Subscriber with Event Bus

## Files

- `event_driven_state_machine.h/cpp` - Core state machine (NO hardware dependencies!)
- `motor_controller.h/cpp` - Example subscribers (motor control + logging)
- `main.cpp` - Example usage with real subscribers
- `event_driven_state_machine_test.cpp` - Unit tests with mock subscribers

## State Machine

```
┌──────┐  Start   ┌──────────────┐  Stop    ┌─────────┐  Reset   ┌──────┐
│ Idle │─────────>│ MotorRunning │─────────>│ Stopped │─────────>│ Idle │
└──────┘          └──────────────┘          └─────────┘          └──────┘
       emit(MotorOn)      │                emit(MotorOff)    emit(SystemReset)
                          │
                          ├─ SpeedUp → emit(MotorSpeedChange)
                          └─ Reverse → emit(MotorDirectionChange)
```

**States:**
- **Idle**: System idle, ready to start
- **MotorRunning**: Motor active, can change speed/direction
- **Stopped**: Motor stopped, can reset to idle

**Input Events:**
- `Start`: Start the motor
- `Stop`: Stop the motor
- `SpeedUp`: Increase motor speed
- `Reverse`: Reverse motor direction
- `Reset`: Reset to idle state

**Output Events:**
- `MotorOn`: Motor should turn on
- `MotorOff`: Motor should turn off
- `MotorSpeedChange`: Motor speed should change
- `MotorDirectionChange`: Motor direction should change
- `SystemReset`: System reset complete

## Key Concepts

### 1. Zero Hardware Dependencies

```cpp
class EventDrivenStateMachine {
    // NO hardware interfaces!
    // NO GPIO references!
    // Only emits events!

    void handleIdle(const InputEvent& event) {
        if (event.type == InputEventType::Start) {
            emit(OutputEventType::MotorOn, MotorConfig{100, true});
            // State machine doesn't know WHO receives this!
        }
    }
};
```

### 2. Type-Safe Events with Enums

```cpp
// Compile-time type safety
enum class InputEventType {
    Start,
    Stop,
    SpeedUp,
    Reverse,
    Reset
};

// No more string comparisons!
switch (event.type) {
    case InputEventType::Start:  // Typo-proof!
        // ...
}
```

### 3. Generic Payloads with std::any

```cpp
// Type-safe payload extraction
struct OutputEvent {
    OutputEventType type;
    std::any payload;  // Can hold ANY type

    template<typename T>
    std::optional<T> getPayload() const;
};

// Usage:
if (auto config = event.getPayload<MotorConfig>()) {
    motor.setSpeed(config->speed);
}
```

### 4. Publisher-Subscriber Pattern

```cpp
// State Machine = Publisher
stateMachine.subscribe([&motor](const OutputEvent& e) {
    motor.handleEvent(e);
});

// Multiple subscribers possible!
stateMachine.subscribe([&logger](const OutputEvent& e) {
    logger.handleEvent(e);
});

stateMachine.subscribe([&telemetry](const OutputEvent& e) {
    telemetry.handleEvent(e);
});
```

## Building and Running

### Example Application

```bash
cd examples/event_driven_state_machine
g++ -std=c++20 -o event_example \
    event_driven_state_machine.cpp \
    motor_controller.cpp \
    main.cpp

./event_example
```

**Expected Output:**
```
========================================
EVENT-DRIVEN STATE MACHINE EXAMPLE
========================================

=== Sending 'Start' event ===
[MotorController] Motor turned ON: 100 RPM, Forward
[EventLogger] Event: MotorOn | Payload: 100 RPM, Forward

=== Sending 'SpeedUp' event ===
[MotorController] Speed changed to: 150 RPM
[EventLogger] Event: MotorSpeedChange | Payload: 150 RPM

=== Sending 'Reverse' event ===
[MotorController] Direction changed: 100 RPM, Reverse
[EventLogger] Event: MotorDirectionChange | Payload: 100 RPM, Reverse

=== Sending 'Stop' event ===
[MotorController] Motor turned OFF
[EventLogger] Event: MotorOff

=== Sending 'Reset' event ===
[MotorController] System reset
[EventLogger] Event: SystemReset
```

### Unit Tests

```bash
g++ -std=c++20 -o event_test \
    event_driven_state_machine.cpp \
    event_driven_state_machine_test.cpp

./event_test
```

**Expected Output:**
```
========================================
EVENT-DRIVEN STATE MACHINE TESTS
========================================

TEST: Idle -> Running transition
  ✓ PASSED

TEST: Speed change while running
  ✓ PASSED

TEST: Direction reversal
  ✓ PASSED

TEST: Full lifecycle (Idle -> Running -> Stopped -> Idle)
  ✓ PASSED

TEST: Invalid transitions are ignored
  ✓ PASSED

TEST: Multiple subscribers receive events
  ✓ PASSED

TEST: Type-safe payload extraction
  ✓ PASSED

========================================
ALL TESTS PASSED ✓
========================================
```

## THE BIG ADVANTAGE: 100% Testable Without Hardware!

### Traditional Embedded Testing Problem:

```cpp
// ❌ Requires ESP32 hardware
TEST(GateTest, opens_on_button) {
    RealGPIO motor;  // Needs actual hardware!
    GateController gate(motor);

    gate.onButton();

    // How to verify? Manual inspection? Oscilloscope?
}
```

### Event-Driven Solution:

```cpp
// ✅ Runs on developer PC in milliseconds!
TEST(GateTest, emits_motor_on_event_on_button) {
    EventDrivenStateMachine sm;
    MockMotorController motor;  // NO hardware!

    sm.subscribe([&motor](const OutputEvent& e) {
        motor.handleEvent(e);
    });

    sm.processEvent(InputEvent{InputEventType::Start});

    // Perfect assertions!
    ASSERT_EQ(motor.getReceivedEvents()[0], OutputEventType::MotorOn);
    ASSERT_TRUE(motor.isMotorRunning());
    ASSERT_EQ(motor.getCurrentSpeed(), 100);
}
```

## Advantages

✅ **ZERO hardware dependencies** - State machine is pure logic
✅ **100% testable on PC** - No ESP32/hardware required
✅ **Multiple subscribers** - One event → many receivers
✅ **Type-safe events** - Enums instead of strings
✅ **Generic payloads** - std::any for flexible data
✅ **Runtime flexibility** - Subscribe/unsubscribe dynamically
✅ **CI/CD friendly** - Standard gcc, runs anywhere
✅ **Fast tests** - Milliseconds instead of minutes

## Comparison to HAL

| Feature | HAL State Machine | Event-Driven State Machine |
|---------|------------------|----------------------------|
| **Hardware Coupling** | Medium (Interfaces) | None (Events) |
| **Testability** | Good (Mock interfaces) | Excellent (Mock subscribers) |
| **Subscribers** | Single | Multiple |
| **Runtime Flexibility** | Static | Dynamic |
| **Test Speed** | Fast | Very Fast |
| **Complexity** | Low | Medium |
| **CI/CD Integration** | Good | Excellent |

## When to Use

Use Event-Driven State Machine when:
- 🎯 **Complex system** with many interacting components
- 🎯 **Multiple outputs** needed for same event (logging, monitoring, telemetry)
- 🎯 **Testability is critical** (automotive, industrial, medical)
- 🎯 **CI/CD without hardware** is required
- 🎯 **Team size > 3** (parallel development without hardware conflicts)

## Architecture Comparison

### HAL Approach:
```
┌──────────────┐
│State Machine │────> motor.setLevel(true)
│ (knows about │
│  hardware)   │
└──────────────┘
```

### Event-Driven Approach:
```
┌──────────────┐
│State Machine │────> emit(MotorOn)
│  (no idea    │           │
│   who gets   │           ├──> Motor Controller
│   events!)   │           ├──> Logger
└──────────────┘           ├──> Telemetry
                           └──> Monitoring
```

## Real-World Use Case: Parking Garage

In the parking garage system, this pattern enables:
- Entry gate emits `BarrierOpen` event
- Motor controller opens physical barrier
- Logger records event
- Display shows "Gate Open"
- Telemetry sends data to cloud
- Monitoring tracks timing

**All from ONE event emission!**

## See Also

- [HAL State Machine Example](../hal_state_machine/) - Simpler interface-based approach
- [Examples Overview](../README.md) - Compare all patterns
