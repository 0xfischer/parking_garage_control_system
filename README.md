# Parking Garage Control System

Event-driven parking garage control system for ESP32 using ESP-IDF and FreeRTOS.

## Features

- **Event-Driven Architecture**: GPIO interrupts trigger events processed by state machines
- **Hardware Abstraction Layer**: Testable design with mock implementations
- **State Machines**: Entry and exit gate controllers with well-defined states
- **Ticket System**: Complete ticket lifecycle management (issue, pay, validate)
- **ESP Console**: Interactive command-line interface for monitoring and control
- **Configurable**: GPIO pins and capacity configurable via Kconfig
- **Thread-Safe**: FreeRTOS mutex and queue protection

## Hardware Configuration

Default GPIO assignment:
- **GPIO 25**: Entry Button (with internal pull-up)
- **GPIO 15**: Entry Light Barrier (with internal pull-up)
- **GPIO 22**: Entry Barrier Servo (PWM via LEDC Channel 0)
- **GPIO 23**: Exit Light Barrier (with internal pull-up)
- **GPIO 2**: Exit Barrier Servo (PWM via LEDC Channel 1)

### Servo Motors
The barrier gates are controlled by servo motors using PWM signals:
- **Frequency**: 50Hz (standard servo)
- **Closed Position (90°)**: 1.5ms pulse width (LOW state) - Barrier vertical
- **Open Position (0°)**: 1ms pulse width (HIGH state) - Barrier horizontal
- **PWM Generation**: ESP32 LEDC (LED Controller) with 14-bit resolution

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    Main Task                         │
│              (Event Loop + Console)                  │
└─────────────────┬───────────────────────────────────┘
                  │
         ┌────────▼────────┐
         │   EventBus      │
         │  (FreeRTOS      │
         │    Queue)       │
         └────┬────┬───────┘
              │    │
      ┌───────┘    └────────┐
      │                     │
┌─────▼──────┐       ┌──────▼──────┐
│   Entry    │       │    Exit     │
│   Gate     │       │    Gate     │
│Controller  │       │ Controller  │
└─────┬──────┘       └──────┬──────┘
      │                     │
      └──────────┬──────────┘
                 │
         ┌───────▼────────┐
         │ Ticket Service │
         └────────────────┘
```

### State Machines

#### Entry Gate State Machine

```mermaid
stateDiagram-v2
    [*] --> Idle

    Idle --> CheckingCapacity : Button Pressed
    CheckingCapacity --> IssuingTicket : Capacity Available
    CheckingCapacity --> Idle : Parking Full
    IssuingTicket --> OpeningBarrier : Ticket Issued
    IssuingTicket --> Idle : Issue Failed
    OpeningBarrier --> WaitingForCar : Barrier Timeout (opened)
    WaitingForCar --> CarPassing : Light Barrier Blocked
    CarPassing --> WaitingBeforeClose : Light Barrier Cleared
    WaitingBeforeClose --> ClosingBarrier : 2 Second Timeout
    ClosingBarrier --> Idle : Barrier Timeout (closed)
```

**States**:
- **Idle**: Waiting for entry button press
- **CheckingCapacity**: Verifying parking availability
- **IssuingTicket**: Creating new ticket for driver
- **OpeningBarrier**: Motor opening barrier (HIGH)
- **WaitingForCar**: Barrier open, waiting for vehicle
- **CarPassing**: Vehicle passing through light barrier
- **WaitingBeforeClose**: 2-second safety delay after car passed
- **ClosingBarrier**: Motor closing barrier (LOW)

**Events**:
- `EntryButtonPressed` → Trigger capacity check
- `CapacityFull` → Reject entry
- `TicketIssued` → Allow entry
- `EntryLightBarrierBlocked` → Car detected
- `EntryLightBarrierCleared` → Car passed
- `BarrierTimeout` → Barrier movement complete

#### Exit Gate State Machine

```mermaid
stateDiagram-v2
    [*] --> Idle

    Idle --> ValidatingTicket : ticket_validate command
    ValidatingTicket --> OpeningBarrier : Validation Success (paid)
    ValidatingTicket --> Idle : Validation Failed (unpaid/not found)
    OpeningBarrier --> WaitingForCarToPass : Barrier Timeout (opened)
    WaitingForCarToPass --> CarPassing : Light Barrier Blocked
    CarPassing --> WaitingBeforeClose : Light Barrier Cleared
    WaitingBeforeClose --> ClosingBarrier : 2 Second Timeout
    ClosingBarrier --> Idle : Barrier Timeout (closed)
```

**States**:
- **Idle**: Waiting for manual ticket validation command
- **ValidatingTicket**: Checking ticket payment status
- **OpeningBarrier**: Motor opening barrier (HIGH)
- **WaitingForCarToPass**: Barrier open, waiting for vehicle
- **CarPassing**: Vehicle passing through light barrier
- **WaitingBeforeClose**: 2-second safety delay after car exited
- **ClosingBarrier**: Motor closing barrier (LOW)

**Events/Commands**:
- `ticket_validate <id>` → Start validation (manual command)
- `TicketValidated` → Ticket is paid, proceed
- `TicketRejected` → Ticket unpaid or invalid, deny exit
- `ExitLightBarrierBlocked` → Car enters barrier area
- `ExitLightBarrierCleared` → Car exited
- `BarrierTimeout` → Barrier movement complete

## Build and Flash

### Prerequisites

- ESP-IDF v5.0 or later
- ESP32 development board

### Build

```bash
# Set up ESP-IDF environment
. $IDF_PATH/export.sh

# Configure project (optional)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

### Configuration

Use `idf.py menuconfig` to configure:
- **GPIO pins**: "Parking Garage Control System Configuration" → "GPIO Configuration"
- **Capacity**: Choose Test Mode (5 spaces) or Production Mode (2000 spaces)
- **Timings**: Barrier timeout, button debounce
  - **Safety Delay**: 2-second wait after car passes before closing barrier (hardcoded)

## Demo
![Demo](demo.gif)

## Console Commands

Available commands in the ESP console:

```
=== Parking Garage Control System ===

  status                    - Show system status
  ticket_list               - List all tickets
  ticket_pay <id>           - Pay ticket
  ticket_validate <id>      - Validate ticket for exit
  gpio_read <gate> <dev>    - Read GPIO state
  simulate_entry            - Simulate entry button press
  simulate_exit             - Simulate car at exit
  ?                         - Show this help
  help                      - Show ESP-IDF help
  clear                     - Clear screen
  restart                   - Restart system
```

### Example Usage

#### Vollständiger Entry/Exit Workflow über Console

Hier ist ein kompletter Durchlauf von Einfahrt bis Ausfahrt. **Wichtig**: Die Schranke öffnet nur bei bezahlten Tickets!

**1. System Status prüfen**
```
parking> status
=== Parking System Status ===
Capacity: 0/5 (5 free)
Entry Gate: Idle
Exit Gate: Idle
Active Tickets: 0
```

**2. Einfahrt simulieren**
```
parking> simulate_entry
Simulating entry button press...
I (1234) EntryGateController: Ticket issued: ID=1
I (1235) EntryGateController: State: Idle -> OpeningBarrier
```
Die State Machine durchläuft automatisch:
Idle → CheckingCapacity → IssuingTicket → OpeningBarrier → WaitingForCar → CarPassing → WaitingBeforeClose (2 Sek) → ClosingBarrier → Idle

**3. Ticket anzeigen lassen**
```
parking> ticket_list
=== Ticket System ===
Active Tickets: 1
Capacity: 5
Available Spaces: 4

Active Tickets:
  Ticket #1: UNPAID (Entry: 2025-12-04 14:23:15)
```

**4. Ticket validieren OHNE Bezahlung**
```
parking> ticket_validate 1
I (5678) ExitGateController: Starting manual ticket validation for ID=1
W (5679) ExitGateController: Ticket not paid: ID=1 - use 'ticket_pay 1' command first!
Error: Failed to validate ticket #1
```
❌ **Validierung fehlgeschlagen!** Das Ticket muss zuerst bezahlt werden.

**5. Ticket bezahlen**
```
parking> ticket_pay 1
Ticket #1 paid successfully

parking> ticket_list
=== Ticket System ===
Active Tickets: 1

Active Tickets:
  Ticket #1: PAID (Entry: 2025-12-04 14:23:15, Paid: 2025-12-04 14:25:32)
```

**6. Ticket validieren MIT Bezahlung**
```
parking> ticket_validate 1
I (7890) ExitGateController: Starting manual ticket validation for ID=1
I (7891) ExitGateController: Ticket validation successful: ID=1
I (7892) ExitGateController: State: ValidatingTicket -> OpeningBarrier
Ticket #1 validated successfully
```
✅ **Schranke öffnet!** Die State Machine durchläuft: Idle → ValidatingTicket → OpeningBarrier → WaitingForCarToPass

**7. Ausfahrt simulieren (Auto fährt durch)**
```
parking> simulate_exit
Simulating car at exit...
I (8000) ExitGateController: Car entering exit barrier
I (8100) ExitGateController: Car exited parking, waiting 2 seconds before closing barrier
I (10100) ExitGateController: Wait period finished, closing barrier
```
Die Light Barrier Events triggern: WaitingForCarToPass → CarPassing → WaitingBeforeClose (2 Sek) → ClosingBarrier → Idle

**8. Status nach Ausfahrt**
```
parking> status
=== Parking System Status ===
Capacity: 0/5 (5 free)
Entry Gate: Idle
Exit Gate: Idle
Active Tickets: 0
```

#### Mehrere Fahrzeuge hintereinander

```
parking> simulate_entry    # Ticket #1 erstellt
parking> simulate_entry    # Ticket #2 erstellt
parking> simulate_entry    # Ticket #3 erstellt

parking> ticket_list
Active Tickets: 3
  Ticket #1: UNPAID
  Ticket #2: UNPAID
  Ticket #3: UNPAID

# Alle Tickets bezahlen
parking> ticket_pay 1
parking> ticket_pay 2
parking> ticket_pay 3

# Fahrzeuge fahren nacheinander raus
parking> ticket_validate 1
parking> simulate_exit

parking> ticket_validate 2
parking> simulate_exit

parking> ticket_validate 3
parking> simulate_exit
```

#### Parkhaus voll

```
parking> status
Capacity: 5/5 (0 free)

parking> simulate_entry
W (9999) EntryGateController: Parking full! (5/5)
```
❌ **Einfahrt verweigert!** Das System geht direkt von CheckingCapacity zurück zu Idle.

## Testing

### Unit Tests

Mock implementations are provided for testing:
- `MockGpioInput`: Simulate GPIO inputs and interrupts
- `MockGpioOutput`: Verify GPIO output states
- `MockEventBus`: Synchronous event processing
- `MockTicketService`: Controllable ticket logic

Example test structure:

```cpp
#include "MockGpioInput.h"
#include "MockGpioOutput.h"
#include "MockEventBus.h"
#include "MockTicketService.h"
#include "EntryGateController.h"

void test_entry_full_cycle() {
    MockEventBus eventBus;
    MockGpioInput button, lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    EntryGateController controller(
        eventBus, button, lightBarrier, motor, tickets, 100
    );

    // Simulate button press
    button.simulateInterrupt(false);  // LOW = pressed
    eventBus.processAllPending();

    // Verify state and motor
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(motor.getLevel() == true);  // Motor ON
}
```
### Example Implementations for Event-Driven State Machines
####  Implementation Using Methods for State Transitions

```cpp
#include <iostream>
#include <string>

class StateMachine {
public:
  enum class State {
    Idle,
    Processing,
    Completed
  };

  StateMachine() : currentState(State::Idle) {}

  void handleEvent(const std::string& event) {
    switch (currentState) {
      case State::Idle:
        onIdle(event);
        break;
      case State::Processing:
        onProcessing(event);
        break;
      case State::Completed:
        onCompleted(event);
        break;
    }
  }

private:
  State currentState;

  void onIdle(const std::string& event) {
    if (event == "start") {
      std::cout << "Transitioning to Processing state\n";
      currentState = State::Processing;
    }
  }

  void onProcessing(const std::string& event) {
    if (event == "complete") {
      std::cout << "Transitioning to Completed state\n";
      currentState = State::Completed;
    }
  }

  void onCompleted(const std::string& event) {
    if (event == "reset") {
      std::cout << "Transitioning to Idle state\n";
      currentState = State::Idle;
    }
  }
};

int main() {
  StateMachine sm;
  sm.handleEvent("start");
  sm.handleEvent("complete");
  sm.handleEvent("reset");
  return 0;
}
```

#### 3. Event-Driven Implementation with HAL Abstraction

This example demonstrates how to decouple logic from hardware using interfaces (HAL). The State Machine receives events and controls hardware via the `IGpioOutput` interface.

[View full example code](examples/hal_state_machine.cpp)

## Project Structure

```
parking_garage_control_system/
├── components/
│   ├── hal/              # Hardware Abstraction Layer
│   │   ├── include/      # GPIO interfaces
│   │   └── src/          # ESP32 implementations
│   ├── events/           # Event system
│   ├── tickets/          # Ticket service
│   ├── gates/            # Gate controllers
│   └── parking/          # Main system orchestrator
├── test/
│   └── mocks/            # Mock implementations
├── main/
│   ├── main.cpp          # Application entry point
│   ├── console_commands.cpp
│   └── Kconfig.projbuild # Configuration menu
└── CMakeLists.txt
```

## C++20 Features Used

- `std::variant` for event payloads
- `std::function` for callbacks
- `std::unique_ptr` for RAII
- `[[nodiscard]]` attribute
- Designated initializers
- `constexpr` where applicable

## License

MIT License

## Author
Eugen Fischer

Created with Claude Code
