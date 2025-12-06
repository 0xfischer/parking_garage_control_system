# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based parking garage control system with:
- Entry gate with button, light barrier, and servo motor
- Exit gate with light barrier and servo motor
- Ticket management system (issue, pay, validate)
- Event-driven state machine architecture
- FreeRTOS-based event bus for decoupled components

## GPIO Pin Configuration

| Component | GPIO Pin | Type | Notes |
|-----------|----------|------|-------|
| Entry Button | GPIO 25 | Input | Active low, debounced |
| Entry Light Barrier | GPIO 23 | Input | HIGH = blocked (car detected) |
| Entry Servo | GPIO 22 | PWM Output | LEDC Channel 0 |
| Exit Light Barrier | GPIO 4 | Input | HIGH = blocked (car detected) |
| Exit Servo | GPIO 2 | PWM Output | LEDC Channel 1 |

**Important**: GPIO 2 has boot restrictions for inputs but works fine for PWM output.

## Build Commands

### ESP32 Build (requires ESP-IDF v5.0+)
```bash
# Set up environment
. $IDF_PATH/export.sh

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### Unit Tests (host machine, no ESP32 needed)
```bash
# Build entry gate tests
g++ -std=c++20 -DUNIT_TEST \
  -I components/parking_system/include \
  -I components/parking_system/include/events \
  -I components/parking_system/include/gates \
  -I components/parking_system/include/hal \
  -I components/parking_system/include/tickets \
  -I test/stubs -I test/mocks \
  -o test/bin_test_entry_gate \
  test/test_entry_gate.cpp \
  components/parking_system/src/gates/EntryGateController.cpp \
  components/parking_system/src/tickets/TicketService.cpp \
  components/parking_system/src/events/FreeRtosEventBus.cpp \
  components/parking_system/src/gates/Gate.cpp

# Build exit gate tests
g++ -std=c++20 -DUNIT_TEST \
  -I components/parking_system/include \
  -I components/parking_system/include/events \
  -I components/parking_system/include/gates \
  -I components/parking_system/include/hal \
  -I components/parking_system/include/tickets \
  -I test/stubs -I test/mocks \
  -o test/bin_test_exit_gate \
  test/test_exit_gate.cpp \
  components/parking_system/src/gates/ExitGateController.cpp \
  components/parking_system/src/tickets/TicketService.cpp \
  components/parking_system/src/events/FreeRtosEventBus.cpp \
  components/parking_system/src/gates/Gate.cpp

# Run tests
./test/bin_test_entry_gate
./test/bin_test_exit_gate
```

### Console Workflow Tests
```bash
g++ -std=c++20 -DUNIT_TEST \
  -I components/parking_system/include \
  -I components/parking_system/include/events \
  -I components/parking_system/include/gates \
  -I components/parking_system/include/hal \
  -I components/parking_system/include/tickets \
  -I test/stubs -I test/mocks \
  -o test/bin_test_console_workflow \
  test/test_console_workflow.cpp \
  test/mocks/ConsoleHarness.cpp \
  main/console_commands.cpp \
  components/parking_system/src/gates/EntryGateController.cpp \
  components/parking_system/src/gates/ExitGateController.cpp \
  components/parking_system/src/tickets/TicketService.cpp \
  components/parking_system/src/events/FreeRtosEventBus.cpp \
  components/parking_system/src/gates/Gate.cpp \
  components/parking_system/src/parking/ParkingGarageSystem.cpp

./test/bin_test_console_workflow
```

## Console Commands

### status
Shows system state (entry/exit gate states, parking capacity, active tickets).
```
status
```

### ticket
Ticket management operations.
```
ticket issue              # Issue new ticket
ticket pay <id>           # Mark ticket as paid
ticket validate <id>      # Validate ticket for exit
ticket list               # List all active tickets
```

### gpio
Read hardware states or write/simulate hardware actions.

**Read operations:**
```
gpio read entry button    # Read entry button state (0/1)
gpio read entry barrier   # Read entry light barrier (blocked/clear)
gpio read entry motor     # Read entry servo position (open/closed)
gpio read exit barrier    # Read exit light barrier (blocked/clear)
gpio read exit motor      # Read exit servo position (open/closed)
```

**Write operations:**
```
gpio write entry motor open   # Open entry barrier
gpio write entry motor close  # Close entry barrier
gpio write exit motor open    # Open exit barrier
gpio write exit motor close   # Close exit barrier
```

**Simulation (publishes events to EventBus):**
```
gpio write entry button pressed   # Simulate button press
gpio write entry barrier blocked  # Simulate car blocking entry
gpio write entry barrier cleared  # Simulate car clearing entry
gpio write exit barrier blocked   # Simulate car blocking exit
gpio write exit barrier cleared   # Simulate car clearing exit
```

### publish
Publish events directly to the EventBus.
```
publish entry_button_pressed
publish entry_light_barrier_blocked
publish entry_light_barrier_cleared
publish exit_light_barrier_blocked
publish exit_light_barrier_cleared
```

### test
Run hardware test sequences (for Wokwi/real hardware).
```
test entry    # Test entry gate flow
test exit     # Test exit gate flow
test full     # Test complete parking flow
```

## Architecture

### Project Structure
```
parking_garage_control_system/
├── main/
│   ├── main.cpp                    # Application entry point
│   ├── console_commands.cpp/.h     # ESP console command handlers
│   └── CMakeLists.txt
├── components/
│   └── parking_system/
│       ├── include/
│       │   ├── events/             # IEventBus, Event, FreeRtosEventBus
│       │   ├── gates/              # IGate, Gate, EntryGateController, ExitGateController
│       │   ├── hal/                # IGpioInput, EspGpioInput, EspServoOutput
│       │   ├── tickets/            # ITicketService, TicketService, Ticket
│       │   └── parking/            # ParkingGarageSystem, ParkingGarageConfig
│       └── src/
│           ├── events/
│           ├── gates/
│           ├── hal/
│           ├── tickets/
│           └── parking/
├── test/
│   ├── mocks/                      # Mock implementations for testing
│   ├── stubs/                      # FreeRTOS/ESP-IDF stubs for host compilation
│   ├── wokwi/                      # Wokwi test scenarios
│   ├── test_entry_gate.cpp
│   ├── test_exit_gate.cpp
│   └── test_console_workflow.cpp
├── diagram.json                    # Wokwi hardware layout
├── wokwi.toml                      # Wokwi configuration
└── .github/workflows/              # CI/CD pipelines
```

### Dual-Constructor Pattern
Controllers have two constructors for testability:

**Production** - creates own hardware from config:
```cpp
EntryGateController(IEventBus&, ITicketService&, EntryGateConfig config);
ExitGateController(IEventBus&, ITicketService&, ExitGateConfig config);
```

**Test** - accepts mock dependencies:
```cpp
EntryGateController(IEventBus&, IGpioInput& button, IGate&, ITicketService&, uint32_t timeout);
ExitGateController(IEventBus&, IGate&, ITicketService&, uint32_t barrierTimeout, uint32_t validationTime);
```

### Ownership Hierarchy
- `ParkingGarageSystem` creates and owns EventBus, TicketService, and gate controllers
- `EntryGateController`/`ExitGateController` own their `Gate` instances (via `m_ownedGate`)
- `Gate` creates and owns hardware (Button, LightBarrier, Motor GPIO/Servo)
- Controllers receive references to shared EventBus and TicketService
- Controllers expose `getGate()` returning `Gate&` for console command access

### Event-Driven State Machines

**Event Types:**
- `ENTRY_BUTTON_PRESSED` - Entry button triggered
- `ENTRY_LIGHT_BARRIER_BLOCKED` - Car detected at entry
- `ENTRY_LIGHT_BARRIER_CLEARED` - Car passed entry
- `EXIT_LIGHT_BARRIER_BLOCKED` - Car detected at exit
- `EXIT_LIGHT_BARRIER_CLEARED` - Car passed exit

**Entry Gate States:**
```
Idle → CheckingCapacity → IssuingTicket → OpeningBarrier → WaitingForCar → CarPassing → WaitingBeforeClose → ClosingBarrier → Idle
```

**Exit Gate States:**
```
Idle → ValidatingTicket → OpeningBarrier → WaitingForCarToPass → CarPassing → WaitingBeforeClose → ClosingBarrier → Idle
```

### Key Interfaces
- `IEventBus`: Publish/subscribe event system with FreeRTOS queue backend
- `IGate`: Hardware abstraction for barrier (open/close, isCarDetected)
- `IGpioInput`: GPIO input with interrupt callback and getLevel()
- `ITicketService`: Ticket lifecycle (issue, pay, validate, remove)

### Gate Class Methods
The `Gate` class (concrete implementation of `IGate`) provides:
```cpp
void open();                    // Open barrier servo
void close();                   // Close barrier servo
bool isOpen() const;            // Check if barrier is open
bool isCarDetected() const;     // Check light barrier state
bool hasButton() const;         // Check if gate has button
EspGpioInput& getButton();      // Access button (entry gate only)
EspGpioInput& getLightBarrier(); // Access light barrier sensor
```

## Testing

### Test Types Overview

| Test Type | Environment | Purpose | Speed |
|-----------|-------------|---------|-------|
| Unit Tests | Host (g++) | State machine logic | Fast |
| Wokwi | Simulation | Hardware integration | Medium |
| Unity | Real ESP32 | Full hardware validation | Slow |

### Test Mocks (test/mocks/)
- `MockEventBus`: Synchronous event processing with `processAllPending()`
- `MockGate`: Tracks open/close state and car detection
- `MockGpioInput`: Simulates button/sensor inputs
- `MockTicketService`: Controllable ticket logic with capacity simulation
- `ConsoleHarness`: Wrapper for testing console commands

### Test Stubs (test/stubs/)
FreeRTOS and ESP-IDF header stubs for host compilation:
- `freertos/FreeRTOS.h`, `freertos/queue.h`, `freertos/timers.h`
- `driver/gpio.h`, `driver/ledc.h`
- `esp_console.h`, `argtable3/argtable3.h`, `linenoise/linenoise.h`

Required defines: `-DUNIT_TEST`

### Test Helpers
Controllers provide test helper methods (only available with `-DUNIT_TEST`):
```cpp
#ifdef UNIT_TEST
void TEST_forceBarrierTimeout();    // Simulate timer expiration
void TEST_forceValidationTimeout(); // For ExitGateController
#endif
```

### Wokwi Simulation
Test scenarios in `test/wokwi/`:
- `entry_flow.yaml` - Complete entry sequence
- `exit_flow.yaml` - Complete exit with paid ticket
- `parking_full.yaml` - Capacity rejection test

Run locally:
```bash
idf.py build
wokwi-cli --scenario test/wokwi/entry_flow.yaml
```

### GitHub Actions CI
- `.github/workflows/wokwi-tests.yml` - Wokwi simulation tests (manual trigger)
- Requires `WOKWI_CLI_TOKEN` secret

## Common Issues

### GPIO 2 Boot Restrictions
GPIO 2 has boot restrictions on ESP32. Use it only for outputs (like servo PWM), not inputs.

### Light Barrier Logic
- HIGH (1) = blocked = car detected
- LOW (0) = clear = no car
- Use `isCarDetected()` to check, not `getLevel()` directly

### Building with Console Commands
Console workflow tests require additional stubs:
- `test/stubs/esp_console.h`
- `test/stubs/argtable3/argtable3.h`
- `test/stubs/linenoise/linenoise.h`
