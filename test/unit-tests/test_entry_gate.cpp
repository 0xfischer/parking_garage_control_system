#include "../mocks/MockGpioInput.h"
#include "../mocks/MockGate.h"
#include "../mocks/MockEventBus.h"
#include "../mocks/MockTicketService.h"
#include "EntryGateController.h"
#include <cassert>
#include <cstdio>

void test_entry_full_cycle();
void test_entry_parking_full();
void test_entry_car_passing();
void test_entry_ignore_repeated_press();

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

// Original implementation follows
#include "../mocks/MockGpioInput.h"
#include "../mocks/MockGate.h"
#include "../mocks/MockEventBus.h"
#include "../mocks/MockTicketService.h"
#include "EntryGateController.h"
#include <cassert>
#include <cstdio>

void test_entry_full_cycle() {
    printf("Test: Entry Full Cycle\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    EntryGateController controller(eventBus, button, gate, tickets, 100);

    assert(controller.getState() == EntryGateState::Idle);
    assert(!gate.isOpen());

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();

    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(gate.isOpen());

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::WaitingForCar);

    eventBus.publish(Event(EventType::EntryLightBarrierBlocked));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::CarPassing);

    eventBus.publish(Event(EventType::EntryLightBarrierCleared));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::WaitingBeforeClose);

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::ClosingBarrier);
    assert(!gate.isOpen());

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::Idle);
}

void test_entry_parking_full() {
    printf("Test: Entry Parking Full\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(2);

    (void) tickets.getNewTicket();
    (void) tickets.getNewTicket();
    assert(tickets.getActiveTicketCount() == 2);

    EntryGateController controller(eventBus, button, gate, tickets, 100);

    assert(controller.getState() == EntryGateState::Idle);

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::Idle);
    assert(!gate.isOpen());
}

void test_entry_car_passing() {
    printf("Test: Entry Car Passing\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    EntryGateController controller(eventBus, button, gate, tickets, 100);

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == EntryGateState::WaitingForCar);
}

void test_entry_ignore_repeated_press() {
    printf("Test: Entry ignores repeated button press\n");

    MockEventBus eventBus;
    MockGpioInput button;
    MockGate gate;
    MockTicketService tickets(5);

    EntryGateController controller(eventBus, button, gate, tickets, 100);

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::OpeningBarrier);

    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();

    assert(tickets.getActiveTicketCount() == 1);
    assert(controller.getState() == EntryGateState::OpeningBarrier);
}
