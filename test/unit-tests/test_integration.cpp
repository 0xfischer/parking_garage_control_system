#include "../mocks/MockGpioInput.h"
#include "../mocks/MockGpioOutput.h"
#include "../mocks/MockEventBus.h"
#include "../mocks/MockTicketService.h"
#include "EntryGateController.h"
#include "ExitGateController.h"
#include <cassert>
#include <cstdio>

void test_entry_does_not_affect_exit();
void test_exit_does_not_affect_entry();

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

void test_entry_does_not_affect_exit() {
    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier;
    MockGpioOutput entryMotor;
    MockGpioInput exitBarrier;
    MockGpioOutput exitMotor;
    MockTicketService tickets(5);

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(entry.getState() == EntryGateState::OpeningBarrier);
    assert(entryMotor.getLevel() == true);

    assert(exitc.getState() == ExitGateState::Idle);
    assert(exitMotor.getLevel() == false);

    entry.TEST_forceBarrierTimeout();
    assert(entry.getState() == EntryGateState::WaitingForCar);

    assert(exitc.getState() == ExitGateState::Idle);
    assert(exitMotor.getLevel() == false);
}

void test_exit_does_not_affect_entry() {
    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier;
    MockGpioOutput entryMotor;
    MockGpioInput exitBarrier;
    MockGpioOutput exitMotor;
    MockTicketService tickets(5);

    uint32_t ticketId = tickets.getNewTicket();
    tickets.payTicket(ticketId);

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    bool validated = exitc.validateTicketManually(ticketId);
    assert(validated);
    eventBus.processAllPending();
    assert(exitc.getState() == ExitGateState::OpeningBarrier);
    assert(exitMotor.getLevel() == true);

    assert(entry.getState() == EntryGateState::Idle);
    assert(entryMotor.getLevel() == false);
}
