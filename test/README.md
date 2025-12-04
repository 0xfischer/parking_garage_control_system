# Parking System Tests

This directory contains test files and mock implementations for unit testing.

## Mock Implementations

Located in `mocks/`:
- **MockGpioInput.h**: Simulate GPIO input pins and interrupts
- **MockGpioOutput.h**: Track GPIO output states
- **MockEventBus.h**: Synchronous event processing for deterministic tests
- **MockTicketService.h**: Controllable ticket system for testing

## Test Files

- **test_entry_gate.cpp**: Entry gate controller tests
- **test_exit_gate.cpp**: Exit gate controller tests (to be implemented)
- **test_ticket_service.cpp**: Ticket service tests (to be implemented)
- **test_integration.cpp**: End-to-end integration tests (to be implemented)

## Running Tests

These tests are designed to run on the host system (not on ESP32).

### Compile and run (example):

```bash
g++ -std=c++20 -I../components/hal/include -I../components/events/include \
    -I../components/tickets/include -I../components/gates/include \
    test_entry_gate.cpp -o test_entry_gate

./test_entry_gate
```

### With CMake (recommended):

Create a separate CMakeLists.txt for host testing with a testing framework like Google Test or Catch2.

## Writing Tests

Example test structure:

```cpp
void test_example() {
    // Setup mocks
    MockEventBus eventBus;
    MockGpioInput input;
    MockGpioOutput output;
    MockTicketService tickets(5);

    // Create controller
    EntryGateController controller(
        eventBus, input, ..., tickets, 100
    );

    // Simulate input
    input.simulateInterrupt(false);
    eventBus.processAllPending();

    // Verify behavior
    assert(controller.getState() == ExpectedState);
    assert(output.getLevel() == expectedLevel);
}
```

## Future Enhancements

- Integration with Google Test or Catch2
- Continuous integration setup
- Code coverage analysis
- Timer simulation for state machine transitions
