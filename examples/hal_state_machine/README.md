# HAL State Machine Example

Hardware Abstraction Layer (HAL) based State Machine for Gate Control.

## Overview

This example demonstrates a state machine that controls hardware through interfaces, enabling testability without actual hardware.

**Architecture Pattern:** Interface-based Dependency Injection

## Files

- `hal_state_machine.h` - State machine and HAL interface definitions
- `hal_state_machine.cpp` - State machine implementation
- `esp32_gpio.h/cpp` - Concrete ESP32 GPIO implementation
- `main.cpp` - Example usage
- `hal_state_machine_test.cpp` - Unit tests with mock GPIO

## State Machine

```
┌─────────┐  ButtonPressed   ┌─────────┐  LimitSwitch   ┌──────┐
│ Closed  │─────────────────>│ Opening │───────────────>│ Open │
└─────────┘                  └─────────┘                └──────┘
              Motor ON                    Motor OFF
```

**States:**
- **Closed**: Gate is closed, motor off
- **Opening**: Gate is opening, motor on
- **Open**: Gate is open, motor off

**Events:**
- `ButtonPressed`: User requests to open gate
- `LimitSwitchReached`: Gate has reached open position

## Key Concepts

### 1. Hardware Abstraction Layer (HAL)

```cpp
// Abstract interface - no hardware dependency
class IGpioOutput {
public:
    virtual void setLevel(bool level) = 0;
    virtual ~IGpioOutput() = default;
};
```

### 2. Dependency Injection

```cpp
// State machine depends on interface, not concrete implementation
class GateController {
public:
    GateController(IGpioOutput& motorGpio);  // Inject dependency
private:
    IGpioOutput& motor;  // Reference to abstraction
};
```

### 3. Testability through Mocking

```cpp
// Production: Real hardware
Esp32Gpio realMotor;
GateController gate(realMotor);

// Testing: Mock hardware
MockGpio mockMotor;
GateController gate(mockMotor);
```

## Building and Running

### Example Application

```bash
cd examples/hal_state_machine
g++ -std=c++17 -o hal_example \
    hal_state_machine.cpp \
    esp32_gpio.cpp \
    main.cpp

./hal_example
```

**Expected Output:**
```
========================================
HAL STATE MACHINE EXAMPLE
========================================

Initial state: Closed

=== Sending ButtonPressed event ===
[GateController] Event: ButtonPressed -> Opening Barrier
[Esp32Gpio] GPIO set to HIGH
Current state: Opening

=== Sending LimitSwitchReached event ===
[GateController] Event: LimitSwitchReached -> Barrier Open
[Esp32Gpio] GPIO set to LOW
Current state: Open
```

### Unit Tests

```bash
g++ -std=c++17 -o hal_test \
    hal_state_machine.cpp \
    hal_state_machine_test.cpp

./hal_test
```

**Expected Output:**
```
========================================
HAL STATE MACHINE TESTS
========================================

TEST: Button press triggers motor ON
  ✓ PASSED

TEST: Limit switch stops motor
  ✓ PASSED

TEST: Full open cycle (Closed -> Opening -> Open)
  ✓ PASSED

TEST: Invalid events are ignored
  ✓ PASSED

TEST: Motor state tracking through transitions
  ✓ PASSED

========================================
ALL TESTS PASSED ✓
========================================
```

## Advantages

✅ **Interface-based abstraction** - Logic decoupled from hardware
✅ **Testable without hardware** - Mock implementations for testing
✅ **Clear separation of concerns** - HAL vs. Logic
✅ **Easy to understand** - Direct method calls
✅ **Type-safe** - Compile-time checking

## Limitations

⚠️ **Direct dependency** - State machine references hardware interface
⚠️ **Single subscriber** - One hardware device per interface
⚠️ **Static binding** - Dependencies set at construction time

## When to Use

Use HAL State Machine when:
- 🎯 Simple 1:1 mapping between state and hardware action
- 🎯 Few hardware components
- 🎯 Direct control flow is preferred
- 🎯 Team is familiar with interface-based design

## Comparison to Event-Driven

For a more decoupled approach with multiple subscribers, see:
[Event-Driven State Machine Example](../event_driven_state_machine/)

| Feature | HAL State Machine | Event-Driven State Machine |
|---------|------------------|----------------------------|
| Coupling | Medium (Interfaces) | Low (Events) |
| Subscribers | Single | Multiple |
| Complexity | Low | Medium |
| Flexibility | Static | Dynamic |
| Best for | Simple systems | Complex systems |
