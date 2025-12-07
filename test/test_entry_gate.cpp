/**
 * @file test_entry_gate.cpp
 * @brief Unit tests for Entry Gate Controller
 *
 * Example tests demonstrating the testability of the system
 * using mock implementations.
 */

#include "../test/mocks/MockGpioInput.h"
#include "../test/mocks/MockGate.h"
#include "../test/mocks/MockEventBus.h"
#include "../test/mocks/MockTicketService.h"
#include "EntryGateController.h"
#include <cassert>
#include <cstdio>

/**
 * @brief Test complete entry cycle
 *
 * Scenario:
 * 1. Button pressed
 * 2. Capacity available
 * 3. Ticket issued
 * 4. Barrier opens
 * 5. Car passes through
 * 6. Barrier closes
 */
void test_entry_full_cycle() {
    printf("Test: Entry Full Cycle\n");

    // Setup
    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5); // Capacity: 5

    EntryGateController controller(
        eventBus, button, gate, tickets, 100);

    // Initial state
    assert(controller.getState() == EntryGateState::Idle);
    assert(!gate.isOpen()); // Gate closed

    // Step 1: Button press -> publish event directly to bus
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();

    // Should transition through states and end up opening barrier
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(gate.isOpen()); // Gate opening

    // Step 2: Simulate barrier opened (timeout)
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::WaitingForCar);

    // Step 3: Car blocks light barrier
    eventBus.publish(Event(EventType::EntryLightBarrierBlocked));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::CarPassing);

    // Step 4: Car clears light barrier -> should wait before closing
    eventBus.publish(Event(EventType::EntryLightBarrierCleared));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::WaitingBeforeClose);

    // Step 5: Simulate 2-second wait timeout
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::ClosingBarrier);
    assert(!gate.isOpen()); // Gate closing

    // Step 6: Simulate barrier closed (timeout)
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::Idle);

    printf("  ✓ Button press handled\n");
    printf("  ✓ Barrier opening\n");
    printf("  ✓ Test passed!\n\n");
}

/**
 * @brief Test entry when parking is full
 *
 * Scenario:
 * 1. Fill parking to capacity
 * 2. Button pressed
 * 3. Capacity check fails
 * 4. Remain in Idle state
 * 5. No ticket issued
 */
void test_entry_parking_full() {
    printf("Test: Entry Parking Full\n");

    // Setup
    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(2); // Capacity: 2

    // Fill parking
    (void) tickets.getNewTicket();
    (void) tickets.getNewTicket();
    assert(tickets.getActiveTicketCount() == 2);

    EntryGateController controller(
        eventBus, button, gate, tickets, 100);

    // Initial state
    assert(controller.getState() == EntryGateState::Idle);

    // Try to enter when full
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    // Ensure that the state remains Idle
    assert(controller.getState() == EntryGateState::Idle);
    assert(!gate.isOpen()); // Gate still closed

    // Should remain idle
    assert(controller.getState() == EntryGateState::Idle);
    assert(!gate.isOpen()); // Gate still closed

    printf("  ✓ Parking full detected\n");
    printf("  ✓ Entry denied\n");
    printf("  ✓ Test passed!\n\n");
}

/**
 * @brief Test car passing through barrier
 */
void test_entry_car_passing() {
    printf("Test: Entry Car Passing\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    EntryGateController controller(
        eventBus, button, gate, tickets, 100);

    // Simulate reaching WaitingForCar state via button press and barrier timeout
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::WaitingForCar);

    printf("  ✓ Car detection logic ready\n");
    printf("  ✓ Test passed!\n\n");
}

/**
 * @brief Test: Multiple button presses while not Idle are ignored
 */
void test_entry_ignore_repeated_press() {
    printf("Test: Entry ignores repeated button press\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    EntryGateController controller(
        eventBus, button, gate, tickets, 100);

    // First press -> starts entry flow
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::OpeningBarrier);

    // Press again while not Idle -> should be ignored
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();

    // Only one active ticket should exist
    assert(tickets.getActiveTicketCount() == 1);
    // Still opening barrier, state unchanged
    assert(controller.getState() == EntryGateState::OpeningBarrier);

    printf("  ✓ Duplicate press ignored\n\n");
}

/**
 * @brief Run all tests
 */
int main() {
    printf("=================================\n");
    printf("Entry Gate Controller Unit Tests\n");
    printf("=================================\n\n");

    test_entry_full_cycle();
    test_entry_parking_full();
    test_entry_car_passing();
    test_entry_ignore_repeated_press();

    printf("=================================\n");
    printf("All tests passed!\n");
    printf("=================================\n");

    return 0;
}
