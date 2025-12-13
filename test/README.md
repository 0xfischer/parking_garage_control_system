# Parking System Tests

This directory contains multiple test approaches for the parking garage system.

## Test Types

| Type | Location | Runs On | Purpose |
|------|----------|---------|---------|
| **Unit Tests (Mocks)** | `test/unit-tests/*.cpp` | Host (PC) | Fast logic testing |
| **Wokwi Simulation** | `test/wokwi-tests/*.yaml` | Wokwi CI | Hardware simulation |
| **Unity HW Tests** | `test/unity-hw-tests/` → mirrors `components/parking_system/test/` | ESP32 | Real hardware |

---

## 1. Unit Tests (Host, Mocks)

Fast tests using mock implementations - no hardware needed.

### Build & Run

```bash
# Entry gate tests
g++ -std=c++20 -DUNIT_TEST \
  -I components/parking_system/include \
  -I components/parking_system/include/events \
  -I components/parking_system/include/gates \
  -I components/parking_system/include/hal \
  -I components/parking_system/include/tickets \
  -I test/stubs -I test/mocks \
    -o test/bin_test_entry_gate \
    test/unit-tests/test_entry_gate.cpp \
  components/parking_system/src/gates/EntryGateController.cpp \
  components/parking_system/src/tickets/TicketService.cpp \
  components/parking_system/src/events/FreeRtosEventBus.cpp \
  components/parking_system/src/gates/Gate.cpp

./test/bin_test_entry_gate

# Exit gate tests
g++ -std=c++20 -DUNIT_TEST \
  -I components/parking_system/include \
  -I components/parking_system/include/events \
  -I components/parking_system/include/gates \
  -I components/parking_system/include/hal \
  -I components/parking_system/include/tickets \
  -I test/stubs -I test/mocks \
    -o test/bin_test_exit_gate \
    test/unit-tests/test_exit_gate.cpp \
  components/parking_system/src/gates/ExitGateController.cpp \
  components/parking_system/src/tickets/TicketService.cpp \
  components/parking_system/src/events/FreeRtosEventBus.cpp \
  components/parking_system/src/gates/Gate.cpp

./test/bin_test_exit_gate
```

### Mock Implementations

Located in `mocks/` (used only by `unit-tests/`):
- **MockEventBus.h**: Synchronous event processing with `processAllPending()`
- **MockGate.h**: Tracks open/close state
- **MockGpioInput.h**: Simulates button/sensor inputs
- **MockGpioOutput.h**: Tracks GPIO output states
- **MockTicketService.h**: Controllable ticket logic
- **ConsoleHarness.h/cpp**: Console command testing

---

## 2. Wokwi Simulation Tests

Hardware simulation with virtual ESP32, buttons, servos, and switches.

### Test Scenarios

| Scenario | File | Description |
|----------|------|-------------|
| Console Full | `wokwi-tests/console_full.yaml` | Complete console-driven workflow |
| Entry/Exit Flow | `wokwi-tests/entry_exit_flow.yaml` | Hardware button + light barrier |
| Parking Full | `wokwi-tests/parking_full.yaml` | Fill parking, test rejection |

### Run Locally

```bash
# Build firmware
idf.py build

# Run with Wokwi CLI
wokwi-cli --scenario test/wokwi-tests/entry_exit_flow.yaml
```

### Run in VS Code

1. Install "Wokwi for VS Code" extension
2. Open `diagram.json`
3. Press F1 → "Wokwi: Start Simulator"

### GitHub Actions

The workflow is configured for **manual trigger only**:
1. Go to Actions → "Wokwi Hardware Tests"
2. Click "Run workflow"
3. Select test scenario

**Setup:** Add `WOKWI_CLI_TOKEN` as repository secret.

---

## 3. Unity Hardware Tests (ESP32)

Tests that run on real ESP32 hardware.

### Build & Flash

```bash
# Build with test component
idf.py -T parking_system build

# Flash and run
idf.py flash monitor
```

### Test Files

Located in `components/parking_system/test/` (mirrored in `test/unity-hw-tests/`):
- **test_entry_gate_hw.cpp**: Entry gate with real GPIO
- **test_exit_gate_hw.cpp**: Exit gate with real GPIO

### Console Test Commands

On the ESP32 console:
```
ParkingGarage> test info     # Show GPIO pin assignments
ParkingGarage> test entry    # Entry gate test guide
ParkingGarage> test exit     # Exit gate test guide
ParkingGarage> test full     # Complete workflow guide
```

---

## GPIO Pin Assignments

| Component | GPIO | Description |
|-----------|------|-------------|
| Entry Button | 25 | Pull LOW to press |
| Entry Light Barrier | 23 | Pull LOW to block |
| Entry Servo | 22 | PWM output |
| Exit Light Barrier | 4 | Pull LOW to block |
| Exit Servo | 2 | PWM output |

---

## Test Stubs

Located in `stubs/` (used only by `unit-tests/`):
- FreeRTOS headers for host compilation
- ESP-IDF driver stubs (GPIO, LEDC)
- Required define: `-DUNIT_TEST`

---

## Architektur für Testbarkeit

Das Projekt verwendet **Dependency Injection** mit **Dual-Constructor Pattern**:

### Production Constructor (erstellt eigene Hardware)
```cpp
EntryGateController entryGate(
    eventBus,
    ticketService,
    EntryGateConfig{
        .buttonPin = GPIO_NUM_25,
        .buttonDebounceMs = 50,
        .lightBarrierPin = GPIO_NUM_23,
        .motorPin = GPIO_NUM_22,
        .ledcChannel = LEDC_CHANNEL_0,
        .barrierTimeoutMs = 2000
    }
);
```

### Test Constructor (akzeptiert Mocks)
```cpp
EntryGateController controller(
    mockEventBus,
    mockButton,
    mockGate,
    mockTicketService,
    2000  // timeout
);
```

---

## Example Test Structure

```cpp
#include "MockGate.h"
#include "MockGpioInput.h"
#include "MockEventBus.h"
#include "MockTicketService.h"
#include "EntryGateController.h"

void test_entry_full_cycle() {
    // Setup mocks
    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    // Create controller with test constructor
    EntryGateController controller(
        eventBus, button, gate, tickets, 100
    );

    // Simulate button press
    Event event(EventType::EntryButtonPressed);
    eventBus.publish(event);
    eventBus.processAllPending();

    // Verify state and gate
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(gate.isOpen() == true);
}
```

---

## Real-World Impact: Testing Without Hardware

### Traditional Embedded Testing
```
❌ Flash code to ESP32 (30+ seconds)
❌ Run test on hardware
❌ Debug via serial monitor
❌ Repeat for each test
⏱️ Total: 10+ minutes per test cycle
```

### Event-Driven Testing
```
✅ Compile on PC (< 1 second)
✅ Run all tests (< 1 second)
✅ Full GDB debugger support
✅ Unlimited parallel tests
⏱️ Total: < 1 second for full test suite
```

**This means:**
- 👥 All developers can test simultaneously (no hardware bottleneck)
- 🚀 200+ test iterations per day instead of ~20
- 💰 No ESP32 boards needed for each developer
- ✅ CI/CD runs on standard GitHub Actions/GitLab CI
