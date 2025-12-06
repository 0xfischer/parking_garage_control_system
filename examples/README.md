# State Machine Examples

This directory contains two complete state machine implementations demonstrating different architectural approaches for embedded systems.

## 📁 Example Structure

```
examples/
├── hal_state_machine/           # Interface-based HAL approach
│   ├── hal_state_machine.h      # State machine + interfaces
│   ├── hal_state_machine.cpp    # Implementation
│   ├── esp32_gpio.h/cpp         # Concrete hardware implementation
│   ├── main.cpp                 # Example usage
│   ├── hal_state_machine_test.cpp   # Unit tests
│   └── README.md                # Documentation
│
└── event_driven_state_machine/  # Event-driven approach
    ├── event_driven_state_machine.h     # State machine (zero hardware deps!)
    ├── event_driven_state_machine.cpp   # Implementation
    ├── motor_controller.h/cpp           # Example subscribers
    ├── main.cpp                         # Example usage
    ├── event_driven_state_machine_test.cpp  # Unit tests
    └── README.md                        # Documentation
```

## 🎯 Quick Start

### HAL State Machine
```bash
cd hal_state_machine
g++ -std=c++20 hal_state_machine.cpp esp32_gpio.cpp main.cpp -o hal_example
./hal_example

# Run tests
g++ -std=c++20 hal_state_machine.cpp hal_state_machine_test.cpp -o hal_test
./hal_test
```

### Event-Driven State Machine
```bash
cd event_driven_state_machine
g++ -std=c++20 event_driven_state_machine.cpp motor_controller.cpp main.cpp -o event_example
./event_example

# Run tests
g++ -std=c++20 event_driven_state_machine.cpp event_driven_state_machine_test.cpp -o event_test
./event_test
```

## 📊 Comparison

### Architecture Diagrams

#### HAL State Machine
```
┌─────────────────┐
│  State Machine  │
│   ┌─────────┐   │
│   │ Logic   │   │
│   └────┬────┘   │
│        │        │
│   ┌────▼──────┐ │
│   │IGpioOutput│ │ Interface
│   └────┬──────┘ │
└────────┼────────┘
         │
    ┌────┴─────┐
    │          │
┌───▼──┐   ┌──▼──┐
│ Real │   │Mock │
│ GPIO │   │GPIO │
└──────┘   └─────┘
```

**Key:** State machine depends on hardware interface

#### Event-Driven State Machine
```
┌─────────────────┐
│  State Machine  │
│   ┌─────────┐   │
│   │ Logic   │   │
│   └────┬────┘   │
│        │        │
│   emit(Event)   │
└────────┼────────┘
         │
    ┌────┼─────────┬──────────┐
    │    │         │          │
┌───▼──┐ │   ┌────▼───┐ ┌────▼────┐
│Motor │ │   │ Logger │ │Telemetry│
└──────┘ │   └────────┘ └─────────┘
     ┌───▼──┐
     │ Mock │ (for tests)
     └──────┘
```

**Key:** State machine knows NOTHING about subscribers

### Feature Comparison

| Feature | HAL State Machine | Event-Driven State Machine |
|---------|------------------|----------------------------|
| **Complexity** | Low ⭐ | Medium ⭐⭐ |
| **Hardware Coupling** | Medium (Interfaces) | None (Events) |
| **Testability** | Good ✓ | Excellent ✓✓ |
| **Multiple Outputs** | Manual | Built-in |
| **Runtime Flexibility** | Static | Dynamic |
| **CI/CD Friendly** | Good | Excellent |
| **Test Speed** | Fast (~ms) | Very Fast (<1ms) |
| **Code Size** | Smaller | Slightly Larger |
| **Learning Curve** | Gentle | Moderate |
| **Best for** | Simple systems | Complex systems |

## 🔍 Detailed Comparison

### Code Examples

#### HAL Approach
```cpp
// State machine has direct dependency
class GateController {
    IGpioOutput& motor;  // Interface reference
public:
    void handleEvent(const Event& event) {
        motor.setLevel(true);  // Direct call
    }
};
```

**Pros:**
- ✅ Simple and direct
- ✅ Easy to understand
- ✅ Less boilerplate

**Cons:**
- ⚠️ State machine coupled to hardware interface
- ⚠️ Single output per interface
- ⚠️ Static binding

#### Event-Driven Approach
```cpp
// State machine has ZERO dependencies
class EventDrivenStateMachine {
    std::vector<EventEmitter> subscribers;
public:
    void handleEvent(const InputEvent& event) {
        emit(OutputEventType::MotorOn, config);  // Broadcast
        // Doesn't know who receives!
    }
};
```

**Pros:**
- ✅ Complete decoupling
- ✅ Multiple subscribers
- ✅ Dynamic subscription
- ✅ Perfect for complex systems

**Cons:**
- ⚠️ More code (events, subscribers)
- ⚠️ Indirection overhead

## 🧪 Testing Comparison

### HAL Testing
```cpp
// Mock the interface
class MockGpio : public IGpioOutput {
    bool currentLevel;
public:
    void setLevel(bool level) override {
        currentLevel = level;  // Record
    }
};

// Test
MockGpio motor;
GateController gate(motor);
gate.handleEvent({ButtonPressed});
assert(motor.getLevel() == true);
```

**Characteristics:**
- ✅ Testable without hardware
- ✅ Mock implements interface
- ⚠️ One mock per interface

### Event-Driven Testing
```cpp
// Mock subscribes to events
class MockMotorController {
    std::vector<OutputEventType> events;
public:
    void handleEvent(const OutputEvent& e) {
        events.push_back(e.type);  // Record
    }
};

// Test
EventDrivenStateMachine sm;
MockMotorController motor;
sm.subscribe([&motor](auto& e) { motor.handleEvent(e); });
sm.processEvent(InputEvent{Start});
assert(motor.getReceivedEvents()[0] == MotorOn);
```

**Characteristics:**
- ✅ Testable without hardware
- ✅ Multiple mock subscribers
- ✅ Easy to add new test observers
- ✅ Perfect event tracing

## 🎓 When to Use Which

### Use HAL State Machine When:
- 🎯 Simple system (1-3 hardware components)
- 🎯 Direct 1:1 state-to-hardware mapping
- 🎯 Team prefers straightforward code
- 🎯 Project timeline is tight
- 🎯 Minimal abstraction overhead desired

**Example Use Cases:**
- Simple motor controller
- LED blinker with states
- Basic sensor reading
- Single actuator control

### Use Event-Driven State Machine When:
- 🎯 **Complex system** with many components
- 🎯 **Multiple subscribers** needed (logging, monitoring, etc.)
- 🎯 **Testability is critical** (automotive, medical, aerospace)
- 🎯 **CI/CD without hardware** required
- 🎯 **Parallel development** (multiple teams)
- 🎯 **Future extensibility** important

**Example Use Cases:**
- **Parking garage system** (gates, tickets, displays, logging)
- Industrial automation
- Vehicle control systems
- Medical devices
- Smart home hubs

## 💡 Key Insights

### HAL: "Tell, Don't Broadcast"
```cpp
motor.setLevel(true);  // Direct command to specific hardware
```
- State machine tells hardware what to do
- 1:1 relationship
- Clear control flow

### Event-Driven: "Broadcast, Don't Care"
```cpp
emit(MotorOn);  // Broadcast event, don't care who handles it
```
- State machine broadcasts intent
- 1:N relationship
- Observers self-organize

## 🚀 Real-World Impact

### Development Workflow Comparison

#### HAL Approach
```
1. Write state machine code
2. Create mock interface
3. Write tests
4. Flash to ESP32 for integration test ⏱️ (10+ min)
5. Debug with serial monitor 🐛
```

#### Event-Driven Approach
```
1. Write state machine code
2. Create mock subscriber
3. Write tests (run on PC) ⚡ (<1 sec)
4. CI/CD auto-tests on commit ✅
5. Flash to ESP32 only for final integration
6. Full debugger support (GDB) 🔍
```

### Team Productivity

**HAL:**
- 👥 2-3 developers share 2 ESP32 boards
- ⏱️ Tests take 5-10 minutes each
- 🔄 1 test at a time per board
- 📊 ~20 test iterations per day

**Event-Driven:**
- 👥 All developers test on their PCs
- ⏱️ Full test suite in <1 second
- 🔄 Unlimited parallel tests
- 📊 ~200+ test iterations per day

### CI/CD Pipeline

**HAL:**
```yaml
# Requires hardware setup
test:
  needs: hardware-runner
  script:
    - flash_to_esp32
    - run_tests
  # 10+ minutes per run
```

**Event-Driven:**
```yaml
# Standard CI (GitHub Actions, GitLab CI)
test:
  runs-on: ubuntu-latest
  script:
    - g++ tests/*.cpp -o test
    - ./test
  # <10 seconds per run
```

## 📚 Learning Path

### Beginner → Intermediate
1. Start with **HAL State Machine**
   - Learn interface-based design
   - Understand dependency injection
   - Master basic testing with mocks

### Intermediate → Advanced
2. Move to **Event-Driven State Machine**
   - Learn publisher-subscriber pattern
   - Master event-based architectures
   - Understand complete decoupling

### Advanced
3. Hybrid approach
   - Use event-driven for complex logic
   - Use HAL for simple peripherals
   - Combine both patterns as needed

## 🔗 Resources

- [HAL State Machine README](hal_state_machine/README.md) - Detailed HAL documentation
- [Event-Driven State Machine README](event_driven_state_machine/README.md) - Detailed event-driven documentation
- [Main Project README](../README.md) - Full parking garage system

## 📝 Summary

Both patterns have their place in embedded development:

**HAL State Machine** = Simplicity + Direct Control
- Perfect for simple systems
- Easy to learn and maintain
- Great for beginners

**Event-Driven State Machine** = Flexibility + Testability
- Perfect for complex systems
- CI/CD without hardware
- Production-grade testing

**The best choice depends on your system complexity and team needs!**

---

*💡 Pro Tip: Start with HAL for your first implementation. If you find yourself needing multiple outputs for the same state transition, or struggling with hardware-dependent tests, migrate to Event-Driven!*

---

## 🎮 Console Workflow Examples

The following examples demonstrate how to use the parking garage system via the ESP console.

### Vollständiger Entry/Exit Workflow

Hier ist ein kompletter Durchlauf von Einfahrt bis Ausfahrt. **Wichtig**: Die Schranke öffnet nur bei bezahlten Tickets!

#### 1. System Status prüfen
```
parking> status
=== Parking System Status ===
Capacity: 0/5 (5 free)
Entry Gate: Idle
Exit Gate: Idle
Active Tickets: 0
```

#### 2. Einfahrt simulieren
```
ParkingGarage> publish EntryButtonPressed
Publishing event: EntryButtonPressed
I (1234) EntryGateController: Ticket issued: ID=1
I (1235) EntryGateController: State: Idle -> OpeningBarrier
```
Die State Machine durchläuft automatisch:
Idle → CheckingCapacity → IssuingTicket → OpeningBarrier → WaitingForCar → CarPassing → WaitingBeforeClose (2 Sek) → ClosingBarrier → Idle

#### 3. Ticket anzeigen lassen
```
ParkingGarage> ticket list
=== Ticket System ===
Active Tickets: 1
Capacity: 5
Available Spaces: 4

Active Tickets:
  Ticket #1: UNPAID (Entry: 2025-12-04 14:23:15)
```

#### 4. Ticket validieren OHNE Bezahlung
```
ParkingGarage> ticket validate 1
I (5678) ExitGateController: Starting manual ticket validation for ID=1
W (5679) ExitGateController: Ticket not paid: ID=1 - use 'ticket pay 1' command first!
Error: Failed to validate ticket #1
```
❌ **Validierung fehlgeschlagen!** Das Ticket muss zuerst bezahlt werden.

#### 5. Ticket bezahlen
```
ParkingGarage> ticket pay 1
Ticket #1 paid successfully

ParkingGarage> ticket list
=== Ticket System ===
Active Tickets: 1

Active Tickets:
  Ticket #1: PAID (Entry: 2025-12-04 14:23:15, Paid: 2025-12-04 14:25:32)
```

#### 6. Ticket validieren MIT Bezahlung
```
ParkingGarage> ticket validate 1
I (7890) ExitGateController: Starting manual ticket validation for ID=1
I (7891) ExitGateController: Ticket validation successful: ID=1
I (7892) ExitGateController: State: ValidatingTicket -> OpeningBarrier
Ticket #1 validated successfully
```
✅ **Schranke öffnet!** Die State Machine durchläuft: Idle → ValidatingTicket → OpeningBarrier → WaitingForCarToPass

#### 7. Ausfahrt simulieren (Auto fährt durch)
```
ParkingGarage> publish ExitLightBarrierBlocked
Publishing event: ExitLightBarrierBlocked
I (8000) ExitGateController: Car entering exit barrier
I (8100) ExitGateController: Car exited parking, waiting 2 seconds before closing barrier
I (10100) ExitGateController: Wait period finished, closing barrier
```
Die Light Barrier Events triggern: WaitingForCarToPass → CarPassing → WaitingBeforeClose (2 Sek) → ClosingBarrier → Idle

#### 8. Status nach Ausfahrt
```
ParkingGarage> status
=== Parking System Status ===
Capacity: 0/5 (5 free)
Entry Gate: Idle
Exit Gate: Idle
Active Tickets: 0
```

### Mehrere Fahrzeuge hintereinander

```
ParkingGarage> publish EntryButtonPressed    # Ticket #1 erstellt
ParkingGarage> publish EntryButtonPressed    # Ticket #2 erstellt
ParkingGarage> publish EntryButtonPressed    # Ticket #3 erstellt

ParkingGarage> ticket list
Active Tickets: 3
  Ticket #1: UNPAID
  Ticket #2: UNPAID
  Ticket #3: UNPAID

# Alle Tickets bezahlen
ParkingGarage> ticket pay 1
ParkingGarage> ticket pay 2
ParkingGarage> ticket pay 3

# Fahrzeuge fahren nacheinander raus
ParkingGarage> ticket validate 1
ParkingGarage> publish ExitLightBarrierBlocked

ParkingGarage> ticket validate 2
ParkingGarage> publish ExitLightBarrierBlocked

ParkingGarage> ticket validate 3
ParkingGarage> publish ExitLightBarrierBlocked
```

### Parkhaus voll

```
ParkingGarage> status
Capacity: 5/5 (0 free)

ParkingGarage> publish EntryButtonPressed
W (9999) EntryGateController: Parking full! (5/5)
```
❌ **Einfahrt verweigert!** Das System geht direkt von CheckingCapacity zurück zu Idle.
