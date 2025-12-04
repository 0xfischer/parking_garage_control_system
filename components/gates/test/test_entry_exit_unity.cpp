#include "unity.h"
#include "EntryGateController.h"
#include "ExitGateController.h"

// Use project mocks for deterministic behavior on target
#include "../../../test/mocks/MockEventBus.h"
#include "../../../test/mocks/MockGpioInput.h"
#include "../../../test/mocks/MockGpioOutput.h"
#include "../../../test/mocks/MockTicketService.h"

TEST_CASE("Entry: full cycle (Unity)", "[entry][unity]") {
    MockEventBus eventBus;
    MockGpioInput button, lightBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);

    EntryGateController entry(eventBus, button, lightBarrier, motor, tickets, 50);

    // Button press
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)EntryGateState::OpeningBarrier, (int)entry.getState());
    TEST_ASSERT_TRUE(motor.getLevel());

    // Barrier opened
    entry.TEST_forceBarrierTimeout();
    TEST_ASSERT_EQUAL((int)EntryGateState::WaitingForCar, (int)entry.getState());

    // Car passes
    eventBus.publish(Event(EventType::EntryLightBarrierBlocked));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)EntryGateState::CarPassing, (int)entry.getState());

    eventBus.publish(Event(EventType::EntryLightBarrierCleared));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)EntryGateState::ClosingBarrier, (int)entry.getState());
    TEST_ASSERT_FALSE(motor.getLevel());

    // Barrier closed
    entry.TEST_forceBarrierTimeout();
    TEST_ASSERT_EQUAL((int)EntryGateState::Idle, (int)entry.getState());
}

TEST_CASE("Exit: full cycle (Unity)", "[exit][unity]") {
    MockEventBus eventBus;
    MockGpioInput exitBarrier;
    MockGpioOutput motor;
    MockTicketService tickets(5);
    // Prepare one active ticket
    (void)tickets.getNewTicket(); // ID=1

    ExitGateController exitc(eventBus, exitBarrier, motor, tickets, 50, 10);

    // Car arrives at exit
    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)ExitGateState::ValidatingTicket, (int)exitc.getState());

    // Validation completes
    exitc.TEST_forceValidationTimeout();
    TEST_ASSERT_EQUAL((int)ExitGateState::OpeningBarrier, (int)exitc.getState());
    TEST_ASSERT_TRUE(motor.getLevel());

    // Barrier opened
    exitc.TEST_forceBarrierTimeout();
    TEST_ASSERT_EQUAL((int)ExitGateState::WaitingForCarToPass, (int)exitc.getState());

    // Car passes and leaves
    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)ExitGateState::CarPassing, (int)exitc.getState());

    eventBus.publish(Event(EventType::ExitLightBarrierCleared));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)ExitGateState::ClosingBarrier, (int)exitc.getState());
    TEST_ASSERT_FALSE(motor.getLevel());

    // Barrier closed
    exitc.TEST_forceBarrierTimeout();
    TEST_ASSERT_EQUAL((int)ExitGateState::Idle, (int)exitc.getState());
}

TEST_CASE("Cross: entry actions do not affect exit (Unity)", "[cross][unity]") {
    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier, exitBarrier;
    MockGpioOutput entryMotor, exitMotor;
    MockTicketService tickets(5);

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    // Trigger entry flow
    eventBus.publish(Event(EventType::EntryButtonPressed));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)EntryGateState::OpeningBarrier, (int)entry.getState());
    TEST_ASSERT_TRUE(entryMotor.getLevel());

    // Ensure exit unaffected
    TEST_ASSERT_EQUAL((int)ExitGateState::Idle, (int)exitc.getState());
    TEST_ASSERT_FALSE(exitMotor.getLevel());
}

TEST_CASE("Cross: exit actions do not affect entry (Unity)", "[cross][unity]") {
    MockEventBus eventBus;
    MockGpioInput entryButton, entryBarrier, exitBarrier;
    MockGpioOutput entryMotor, exitMotor;
    MockTicketService tickets(5);

    // Prepare a ticket so exit validation succeeds
    (void)tickets.getNewTicket();

    EntryGateController entry(eventBus, entryButton, entryBarrier, entryMotor, tickets, 50);
    ExitGateController exitc(eventBus, exitBarrier, exitMotor, tickets, 50, 10);

    // Trigger exit flow
    eventBus.publish(Event(EventType::ExitLightBarrierBlocked));
    eventBus.processAllPending();
    TEST_ASSERT_EQUAL((int)ExitGateState::ValidatingTicket, (int)exitc.getState());

    exitc.TEST_forceValidationTimeout();
    TEST_ASSERT_EQUAL((int)ExitGateState::OpeningBarrier, (int)exitc.getState());
    TEST_ASSERT_TRUE(exitMotor.getLevel());

    // Ensure entry unaffected
    TEST_ASSERT_EQUAL((int)EntryGateState::Idle, (int)entry.getState());
    TEST_ASSERT_FALSE(entryMotor.getLevel());
}
