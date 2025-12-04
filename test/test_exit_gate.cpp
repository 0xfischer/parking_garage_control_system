/**
 * @file test_exit_gate.cpp
 * @brief Unit tests for Exit Gate Controller
 */

#include "../test/mocks/MockGpioInput.h"
#include "../test/mocks/MockGpioOutput.h"
#include "../test/mocks/MockEventBus.h"
#include "../test/mocks/MockTicketService.h"
#include "ExitGateController.h"
#include <cassert>
#include <cstdio>

void test_exit_full_cycle() {
    printf("Test: Exit Full Cycle (Manual Validation)\n");

    MockEventBus eventBus;
    MockGpioInput lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    // Pre-create one active PAID ticket (ID=1)
    uint32_t id = tickets.getNewTicket();
    assert(id == 1);
    tickets.payTicket(id);  // Pay the ticket!

    ExitGateController controller(
        eventBus, lightBarrier, motor, tickets, 100, 50
    );

    // Initial state
    assert(controller.getState() == ExitGateState::Idle);
    assert(motor.getLevel() == false);

    // Manually validate ticket
    bool validated = controller.validateTicketManually(id);
    assert(validated);
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::OpeningBarrier);
    assert(motor.getLevel() == true);

    // Barrier finished opening
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::WaitingForCarToPass);

    // Car passes through
    eventBus.publish(Event(EventType::ExitLightBarrierBlocked)); // blocked -> entering
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::CarPassing);

    eventBus.publish(Event(EventType::ExitLightBarrierCleared));  // cleared -> car left
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::WaitingBeforeClose);

    // Wait 2 seconds before closing
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::ClosingBarrier);
    assert(motor.getLevel() == false);

    // Barrier finished closing
    controller.TEST_forceBarrierTimeout();
    assert(controller.getState() == ExitGateState::Idle);

    printf("  ✓ Exit full cycle with manual validation\n\n");
}

void test_exit_no_tickets_rejected() {
    printf("Test: Exit No Tickets -> Rejected\n");

    MockEventBus eventBus;
    MockGpioInput lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    ExitGateController controller(
        eventBus, lightBarrier, motor, tickets, 100, 10
    );

    // Try to validate non-existent ticket
    bool validated = controller.validateTicketManually(99);
    assert(!validated);  // Should fail
    eventBus.processAllPending();

    // Should return to Idle (rejected)
    assert(controller.getState() == ExitGateState::Idle);
    assert(motor.getLevel() == false);

    printf("  ✓ Rejection without active tickets\n\n");
}

void test_exit_unpaid_ticket_rejected() {
    printf("Test: Exit rejects unpaid ticket\n");

    MockEventBus eventBus;
    MockGpioInput lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    // Prepare one active ticket (UNPAID)
    uint32_t id = tickets.getNewTicket(); // ID=1
    Ticket ticket;
    bool found = tickets.getTicketInfo(id, ticket);
    assert(found && !ticket.isPaid);  // Verify unpaid

    ExitGateController controller(
        eventBus, lightBarrier, motor, tickets, 100, 50
    );

    // Try to validate unpaid ticket
    bool validated = controller.validateTicketManually(id);
    assert(!validated);  // Should fail - ticket not paid
    eventBus.processAllPending();

    // Should return to Idle
    assert(controller.getState() == ExitGateState::Idle);
    assert(motor.getLevel() == false);

    // Ensure no ExitBarrierOpened event
    bool openedEvent = false;
    for (const auto& ev : eventBus.history()) {
        if (ev.type == EventType::ExitBarrierOpened) { openedEvent = true; break; }
    }
    assert(!openedEvent && "ExitBarrierOpened should not be published for unpaid ticket");

    printf("  ✓ Unpaid ticket rejected\n\n");
}

void test_exit_light_barrier_in_idle_ignored() {
    printf("Test: Light barrier in Idle state is ignored\n");

    MockEventBus eventBus;
    MockGpioInput lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    // Create a paid ticket
    uint32_t id = tickets.getNewTicket();
    tickets.payTicket(id);

    ExitGateController controller(
        eventBus, lightBarrier, motor, tickets, 100, 10
    );

    // Initial state
    assert(controller.getState() == ExitGateState::Idle);

    // Trigger light barrier in Idle - should be ignored
    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();

    // Should still be in Idle
    assert(controller.getState() == ExitGateState::Idle);
    assert(motor.getLevel() == false);

    printf("  ✓ Light barrier ignored in Idle state\n\n");
}

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
