/**
 * @file test_integration.cpp
 * @brief Integration tests: Entry and Exit controllers on same event bus
 *        Cross-check that entry actions don't affect exit, and vice versa.
 */

#include "../test/mocks/MockGpioInput.h"
#include "../test/mocks/MockGpioOutput.h"
#include "../test/mocks/MockEventBus.h"
#include "../test/mocks/MockTicketService.h"
#include "EntryGateController.h"
#include "ExitGateController.h"
#include <cassert>
#include <cstdio>

void test_entry_does_not_affect_exit() {
    printf("Test: Entry does not affect Exit\n");

    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier;
    MockGpioOutput entryMotor;
    MockGpioInput exitBarrier;
    MockGpioOutput exitMotor;
    MockTicketService tickets(5);

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    // Trigger entry flow
    // Publish entry button press on event bus
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(entry.getState() == EntryGateState::OpeningBarrier);
    assert(entryMotor.getLevel() == true);

    // Exit must remain idle and motor off
    assert(exitc.getState() == ExitGateState::Idle);
    assert(exitMotor.getLevel() == false);

    // Advance entry to waiting for car
    entry.TEST_forceBarrierTimeout();
    assert(entry.getState() == EntryGateState::WaitingForCar);

    // Cross: still no change at exit
    assert(exitc.getState() == ExitGateState::Idle);
    assert(exitMotor.getLevel() == false);

    printf("  ✓ No cross-effects from entry to exit\n\n");
}

void test_exit_does_not_affect_entry() {
    printf("Test: Exit does not affect Entry\n");

    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier;
    MockGpioOutput entryMotor;
    MockGpioInput exitBarrier;
    MockGpioOutput exitMotor;
    MockTicketService tickets(5);

    // Prepare a PAID ticket so exit validation can succeed
    uint32_t ticketId = tickets.getNewTicket(); // ID=1
    tickets.payTicket(ticketId);

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    // Trigger exit flow via manual validation
    bool validated = exitc.validateTicketManually(ticketId);
    assert(validated);
    eventBus.processAllPending();
    assert(exitc.getState() == ExitGateState::OpeningBarrier);
    assert(exitMotor.getLevel() == true);

    // Entry must remain idle and motor off
    assert(entry.getState() == EntryGateState::Idle);
    assert(entryMotor.getLevel() == false);

    printf("  ✓ No cross-effects from exit to entry\n\n");
}

int main() {
    printf("=================================\n");
    printf("Integration Tests (Entry <-> Exit)\n");
    printf("=================================\n\n");

    test_entry_does_not_affect_exit();
    test_exit_does_not_affect_entry();

    printf("=================================\n");
    printf("All integration tests passed!\n");
    printf("=================================\n");
    return 0;
}
