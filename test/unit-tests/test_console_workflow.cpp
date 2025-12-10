#include <cassert>
#include <iostream>
#include <string>
#include "freertos/FreeRTOS.h"
#include "MockEventBus.h"
#include "MockTicketService.h"
#include "MockGate.h"
#include "MockGpioInput.h"
#include "ParkingGarageSystem.h"
#include "EntryGateController.h"
#include "ExitGateController.h"
#include "ConsoleHarness.h"

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"

void printTestHeader(const char* testName) {
    std::cout << COLOR_BLUE << "\n=== Test: " << testName << " ===" << COLOR_RESET << "\n";
}
void printSuccess(const char* msg) {
    std::cout << COLOR_GREEN << "✓ " << msg << COLOR_RESET << "\n";
}
void printInfo(const char* msg) {
    std::cout << "  " << msg << "\n";
}

void test_complete_entry_flow();
void test_complete_exit_flow_paid();
void test_exit_flow_unpaid_rejected();
void test_entry_flow_parking_full();
void test_multiple_vehicles_sequential();
void test_full_workflow_entry_to_exit();

int main() {
    std::cout << COLOR_BLUE << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Console Workflow Integration Tests                   ║\n";
    std::cout << "║  Testing State Machine Flows via Console Commands     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET << "\n";

    try {
        test_complete_entry_flow();
        test_complete_exit_flow_paid();
        test_exit_flow_unpaid_rejected();
        test_entry_flow_parking_full();
        test_multiple_vehicles_sequential();
        test_full_workflow_entry_to_exit();

        std::cout << COLOR_GREEN << "\n";
        std::cout << "╔════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✓ All Console Workflow Tests Passed! (6/6)           ║\n";
        std::cout << "╚════════════════════════════════════════════════════════╝\n";
        std::cout << COLOR_RESET << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "Test failed with exception: " << e.what() << COLOR_RESET << "\n";
        return 1;
    }
}

void test_complete_entry_flow() {
    printTestHeader("Complete Entry Flow (publish EntryButtonPressed)");
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockTicketService ticketService(5);
    ParkingGarageSystem system(eventBus, ticketService, entryGate, entryButton, entryLightBarrier);
    console_test_init(&system);
    auto& controller = system.getEntryGate();
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("Initial state: Idle");
    printInfo("Command: publish EntryButtonPressed");
    run_console_command("publish EntryButtonPressed");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::CheckingCapacity);
    printInfo("State: CheckingCapacity");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::IssuingTicket);
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("State: IssuingTicket (Ticket #1 issued)");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(entryGate.isOpen() == true);
    printInfo("State: OpeningBarrier (barrier opens)");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::WaitingForCar);
    printInfo("State: WaitingForCar");
    printInfo("Car enters light barrier");
    run_console_command("publish EntryLightBarrierBlocked");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::CarPassing);
    printInfo("State: CarPassing");
    printInfo("Car clears light barrier");
    run_console_command("publish EntryLightBarrierCleared");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::WaitingBeforeClose);
    printInfo("State: WaitingBeforeClose (2 sec delay)");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::ClosingBarrier);
    assert(entryGate.isOpen() == false);
    printInfo("State: ClosingBarrier (barrier closes)");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("State: Idle (complete cycle)");
    printSuccess("Complete entry flow works correctly!");
}

void test_complete_exit_flow_paid() {
    printTestHeader("Complete Exit Flow with Paid Ticket");
    MockEventBus eventBus;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);
    ParkingGarageSystem system(eventBus, ticketService, exitGate, /*entryButton*/ exitLightBarrier /*placeholder*/, exitLightBarrier);
    console_test_init(&system);
    uint32_t ticketId = system.getTicketService().getNewTicket();
    run_console_command(std::string("ticket pay ") + std::to_string(ticketId));
    printInfo("Ticket #1 created and paid");
    auto& controller = system.getExitGate();
    assert(controller.getState() == ExitGateState::Idle);
    printInfo("Initial state: Idle");
    printInfo("Command: ticket validate 1");
    int rc = run_console_command(std::string("ticket validate ") + std::to_string(ticketId));
    assert(rc == 0);
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::ValidatingTicket);
    printInfo("State: ValidatingTicket");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::OpeningBarrier);
    assert(exitGate.isOpen() == true);
    printInfo("State: OpeningBarrier (barrier opens)");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::WaitingForCarToPass);
    printInfo("State: WaitingForCarToPass");
    printInfo("Command: publish ExitLightBarrierBlocked");
    run_console_command("publish ExitLightBarrierBlocked");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::CarPassing);
    printInfo("State: CarPassing");
    printInfo("Car clears light barrier");
    run_console_command("publish ExitLightBarrierCleared");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::WaitingBeforeClose);
    printInfo("State: WaitingBeforeClose (2 sec delay)");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::ClosingBarrier);
    assert(exitGate.isOpen() == false);
    printInfo("State: ClosingBarrier (barrier closes)");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("State: Idle (ticket used, complete cycle)");
    printSuccess("Complete exit flow with paid ticket works correctly!");
}

void test_exit_flow_unpaid_rejected() {
    printTestHeader("Exit Flow Rejected - Unpaid Ticket");
    MockEventBus eventBus;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);
    uint32_t ticketId = ticketService.getNewTicket();
    printInfo("Ticket #1 created (UNPAID)");
    ExitGateController controller(eventBus, exitGate, ticketService, 100);
    assert(controller.getState() == ExitGateState::Idle);
    printInfo("Initial state: Idle");
    printInfo("Command: ticket validate 1 (unpaid)");
    bool validated = controller.validateTicketManually(ticketId);
    assert(validated == false);
    printInfo("Validation rejected: Ticket not paid");
    eventBus.processAllPending();
    assert(controller.getState() == ExitGateState::Idle);
    assert(exitGate.isOpen() == false);
    printInfo("State: Idle (barrier remains closed)");
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("Ticket still active (not used)");
    printSuccess("Exit flow correctly rejects unpaid ticket!");
}

void test_entry_flow_parking_full() {
    printTestHeader("Entry Flow Rejected - Parking Full");
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockTicketService ticketService(2);
    ticketService.getNewTicket();
    ticketService.getNewTicket();
    assert(ticketService.getActiveTicketCount() == 2);
    printInfo("Parking full: 2/2 spaces occupied");
    EntryGateController controller(eventBus, entryButton, entryGate, ticketService, 100);
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("Initial state: Idle");
    printInfo("Command: publish EntryButtonPressed (parking full)");
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::CheckingCapacity);
    printInfo("State: CheckingCapacity (checking capacity)");
    eventBus.processAllPending();
    assert(controller.getState() == EntryGateState::Idle);
    assert(entryGate.isOpen() == false);
    printInfo("State: Idle (entry rejected, barrier closed)");
    assert(ticketService.getActiveTicketCount() == 2);
    printInfo("No new ticket issued");
    printSuccess("Entry flow correctly rejects when parking is full!");
}

void test_multiple_vehicles_sequential() {
    printTestHeader("Multiple Vehicles Sequential Flow");
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);
    EntryGateController entryController(eventBus, entryButton, entryGate, ticketService, 100);
    ExitGateController exitController(eventBus, exitGate, ticketService, 100);
    for (int i = 1; i <= 3; i++) {
        printInfo((std::string("Car ") + std::to_string(i) + " entering...").c_str());
        Event buttonEvent(EventType::EntryButtonPressed);
        eventBus.publish(buttonEvent);
        eventBus.processAllPending();
        eventBus.processAllPending();
        eventBus.processAllPending();
        eventBus.processAllPending();
        Event blocked(EventType::EntryLightBarrierBlocked);
        eventBus.publish(blocked);
        eventBus.processAllPending();
        Event cleared(EventType::EntryLightBarrierCleared);
        eventBus.publish(cleared);
        eventBus.processAllPending();
        eventBus.processAllPending();
        eventBus.processAllPending();
        assert(entryController.getState() == EntryGateState::Idle);
    }
    assert(ticketService.getActiveTicketCount() == 3);
    printInfo("3 cars entered successfully");
    for (uint32_t id = 1; id <= 3; id++) {
        ticketService.payTicket(id);
    }
    printInfo("All tickets paid");
    for (uint32_t id = 1; id <= 3; id++) {
        printInfo((std::string("Car ") + std::to_string(id) + " exiting...").c_str());
        bool validated = exitController.validateTicketManually(id);
        assert(validated == true);
        eventBus.processAllPending();
        eventBus.processAllPending();
        eventBus.processAllPending();
        Event blocked(EventType::ExitLightBarrierBlocked);
        eventBus.publish(blocked);
        eventBus.processAllPending();
        Event cleared(EventType::ExitLightBarrierCleared);
        eventBus.publish(cleared);
        eventBus.processAllPending();
        eventBus.processAllPending();
        eventBus.processAllPending();
        assert(exitController.getState() == ExitGateState::Idle);
    }
    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("All 3 cars exited successfully");
    printSuccess("Multiple vehicles sequential flow works correctly!");
}

void test_full_workflow_entry_to_exit() {
    printTestHeader("Full Workflow - Entry to Exit");
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);
    EntryGateController entryController(eventBus, entryButton, entryGate, ticketService, 100);
    ExitGateController exitController(eventBus, exitGate, ticketService, 100);
    printInfo("Initial: 0 tickets, parking empty");
    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("Step 1: publish EntryButtonPressed");
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    eventBus.processAllPending();
    eventBus.processAllPending();
    uint32_t ticketId = ticketService.getActiveTicketCount();
    eventBus.processAllPending();
    eventBus.processAllPending();
    Event entryBlocked(EventType::EntryLightBarrierBlocked);
    eventBus.publish(entryBlocked);
    eventBus.processAllPending();
    Event entryCleared(EventType::EntryLightBarrierCleared);
    eventBus.publish(entryCleared);
    eventBus.processAllPending();
    eventBus.processAllPending();
    eventBus.processAllPending();
    assert(entryController.getState() == EntryGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("Entry complete: Ticket #1 issued");
    printInfo("Step 2: ticket pay 1");
    bool paid = ticketService.payTicket(ticketId);
    assert(paid == true);
    printInfo("Ticket #1 paid");
    printInfo("Step 3: ticket validate 1");
    bool validated = exitController.validateTicketManually(ticketId);
    assert(validated == true);
    eventBus.processAllPending();
    eventBus.processAllPending();
    eventBus.processAllPending();
    printInfo("Step 4: publish ExitLightBarrierBlocked");
    Event exitBlocked(EventType::ExitLightBarrierBlocked);
    eventBus.publish(exitBlocked);
    eventBus.processAllPending();
    Event exitCleared(EventType::ExitLightBarrierCleared);
    eventBus.publish(exitCleared);
    eventBus.processAllPending();
    eventBus.processAllPending();
    eventBus.processAllPending();
    assert(exitController.getState() == ExitGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("Exit complete: Parking empty");
    printSuccess("Full workflow from entry to exit works correctly!");
}
