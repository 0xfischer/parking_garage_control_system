#include "../mocks/MockGate.h"
#include "../mocks/MockEventBus.h"
#include "../mocks/MockTicketService.h"
#include "ExitGateController.h"
#include <cassert>
#include <cstdio>

void test_exit_full_cycle();
void test_exit_no_tickets_rejected();
void test_exit_unpaid_ticket_rejected();
void test_exit_light_barrier_in_idle_ignored();

int main() {
    printf("=================================\n");
    printf("Exit Gate Controller Unit Tests\n");
    printf("=================================\n\n");

    test_exit_full_cycle();
    test_exit_no_tickets_rejected();
    test_exit_unpaid_ticket_rejected();
    test_exit_light_barrier_in_idle_ignored();

    printf("=================================\n");
    printf("All tests passed!\n");
    printf("=================================\n");
    return 0;
}

// Original implementations
#include "../mocks/MockGate.h"
#include "../mocks/MockEventBus.h"
#include "../mocks/MockTicketService.h"
#include "ExitGateController.h"
#include <cassert>
#include <cstdio>

void test_exit_full_cycle() {
    MockEventBus eventBus;
    MockGate gate;
    MockTicketService tickets(5);

    uint32_t id = tickets.getNewTicket();
    tickets.payTicket(id);

    ExitGateController controller(eventBus, gate, tickets, 100, 50);
    assert(controller.getState() == ExitGateState::Idle);
    assert(!gate.isOpen());

    bool validated = controller.validateTicketManually(id);
    assert(validated);
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::OpeningBarrier);
    assert(gate.isOpen());

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::WaitingForCarToPass);

    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::CarPassing);

    eventBus.publish(Event(EventType::ExitLightBarrierCleared));
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::WaitingBeforeClose);

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::ClosingBarrier);
    assert(!gate.isOpen());

    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::Idle);
}

void test_exit_no_tickets_rejected() {
    MockEventBus eventBus;
    MockGate gate;
    MockTicketService tickets(5);

    ExitGateController controller(eventBus, gate, tickets, 100, 10);
    bool validated = controller.validateTicketManually(99);
    assert(!validated);
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::Idle);
    assert(!gate.isOpen());
}

void test_exit_unpaid_ticket_rejected() {
    MockEventBus eventBus;
    MockGate gate;
    MockTicketService tickets(5);

    uint32_t id = tickets.getNewTicket();
    Ticket ticket;
    bool found = tickets.getTicketInfo(id, ticket);
    assert(found && !ticket.isPaid);

    ExitGateController controller(eventBus, gate, tickets, 100, 50);
    bool validated = controller.validateTicketManually(id);
    assert(!validated);
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::Idle);
    assert(!gate.isOpen());
}

void test_exit_light_barrier_in_idle_ignored() {
    MockEventBus eventBus;
    MockGate gate;
    MockTicketService tickets(5);

    uint32_t id = tickets.getNewTicket();
    tickets.payTicket(id);

    ExitGateController controller(eventBus, gate, tickets, 100, 10);
    assert(controller.getState() == ExitGateState::Idle);

    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();

    assert(controller.getState() == ExitGateState::Idle);
    assert(!gate.isOpen());
}
