/**
 * @file test_console_workflow.cpp
 * @brief Integration tests for complete parking garage workflows via console commands
 *
 * These tests simulate the complete state machine flows that users would trigger
 * through the console interface, validating end-to-end behavior.
 */

#include <cassert>
#include <iostream>
#include <string>

// Test stubs and mocks
#include "freertos/FreeRTOS.h"
#include "MockEventBus.h"
#include "MockTicketService.h"
#include "MockGate.h"
#include "MockGpioInput.h"

// Production code
#include "ParkingGarageSystem.h"
#include "EntryGateController.h"
#include "ExitGateController.h"
// Console harness
#include "ConsoleHarness.h"

// Color codes for output
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

/**
 * Test 1: Complete Entry Flow
 * Simulates: publish EntryButtonPressed
 * Expected: Ticket issued, barrier opens and closes, returns to Idle
 */
void test_complete_entry_flow() {
    printTestHeader("Complete Entry Flow (publish EntryButtonPressed)");

    // Setup system using mocks
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockTicketService ticketService(5); // Capacity: 5
    ParkingGarageSystem system(eventBus, ticketService, entryGate, entryButton, entryLightBarrier);
    console_test_init(&system);
    auto& controller = system.getEntryGate();

    // Initial state
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("Initial state: Idle");

    // Simulate: publish EntryButtonPressed
    printInfo("Command: publish EntryButtonPressed");
    run_console_command("publish EntryButtonPressed");
    eventBus.processAllPending();

    // State: CheckingCapacity
    assert(controller.getState() == EntryGateState::CheckingCapacity);
    printInfo("State: CheckingCapacity");
    eventBus.processAllPending();

    // State: IssuingTicket
    assert(controller.getState() == EntryGateState::IssuingTicket);
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("State: IssuingTicket (Ticket #1 issued)");
    eventBus.processAllPending();

    // State: OpeningBarrier
    assert(controller.getState() == EntryGateState::OpeningBarrier);
    assert(entryGate.isOpen() == true);
    printInfo("State: OpeningBarrier (barrier opens)");
    eventBus.processAllPending();

    // State: WaitingForCar
    assert(controller.getState() == EntryGateState::WaitingForCar);
    printInfo("State: WaitingForCar");

    // Simulate: Car enters (light barrier blocked)
    printInfo("Car enters light barrier");
    run_console_command("publish EntryLightBarrierBlocked");
    eventBus.processAllPending();

    // State: CarPassing
    assert(controller.getState() == EntryGateState::CarPassing);
    printInfo("State: CarPassing");

    // Simulate: Car clears barrier
    printInfo("Car clears light barrier");
    run_console_command("publish EntryLightBarrierCleared");
    eventBus.processAllPending();

    // State: WaitingBeforeClose
    assert(controller.getState() == EntryGateState::WaitingBeforeClose);
    printInfo("State: WaitingBeforeClose (2 sec delay)");
    eventBus.processAllPending();

    // State: ClosingBarrier
    assert(controller.getState() == EntryGateState::ClosingBarrier);
    assert(entryGate.isOpen() == false);
    printInfo("State: ClosingBarrier (barrier closes)");
    eventBus.processAllPending();

    // Final state: Idle
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("State: Idle (complete cycle)");

    printSuccess("Complete entry flow works correctly!");
}

/**
 * Test 2: Complete Exit Flow with Paid Ticket
 * Simulates: ticket validate 1 → publish ExitLightBarrierBlocked
 * Expected: Barrier opens, car passes, barrier closes
 */
void test_complete_exit_flow_paid() {
    printTestHeader("Complete Exit Flow with Paid Ticket");

    // Setup
    MockEventBus eventBus;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);
    ParkingGarageSystem system(eventBus, ticketService, exitGate, /*entryButton*/exitLightBarrier /*placeholder*/, exitLightBarrier);
    console_test_init(&system);
    // Create a paid ticket via system service
    uint32_t ticketId = system.getTicketService().getNewTicket();
    run_console_command(std::string("ticket pay ") + std::to_string(ticketId));
    printInfo("Ticket #1 created and paid");

    auto& controller = system.getExitGate();

    // Initial state
    assert(controller.getState() == ExitGateState::Idle);
    printInfo("Initial state: Idle");

    // Simulate: ticket validate 1
    printInfo("Command: ticket validate 1");
    int rc = run_console_command(std::string("ticket validate ") + std::to_string(ticketId));
    assert(rc == 0);
    eventBus.processAllPending();

    // State: ValidatingTicket
    assert(controller.getState() == ExitGateState::ValidatingTicket);
    printInfo("State: ValidatingTicket");
    eventBus.processAllPending();

    // State: OpeningBarrier
    assert(controller.getState() == ExitGateState::OpeningBarrier);
    assert(exitGate.isOpen() == true);
    printInfo("State: OpeningBarrier (barrier opens)");
    eventBus.processAllPending();

    // State: WaitingForCarToPass
    assert(controller.getState() == ExitGateState::WaitingForCarToPass);
    printInfo("State: WaitingForCarToPass");

    // Simulate: publish ExitLightBarrierBlocked
    printInfo("Command: publish ExitLightBarrierBlocked");
    run_console_command("publish ExitLightBarrierBlocked");
    eventBus.processAllPending();

    // State: CarPassing
    assert(controller.getState() == ExitGateState::CarPassing);
    printInfo("State: CarPassing");

    // Simulate: Car clears barrier
    printInfo("Car clears light barrier");
    run_console_command("publish ExitLightBarrierCleared");
    eventBus.processAllPending();

    // State: WaitingBeforeClose
    assert(controller.getState() == ExitGateState::WaitingBeforeClose);
    printInfo("State: WaitingBeforeClose (2 sec delay)");
    eventBus.processAllPending();

    // State: ClosingBarrier
    assert(controller.getState() == ExitGateState::ClosingBarrier);
    assert(exitGate.isOpen() == false);
    printInfo("State: ClosingBarrier (barrier closes)");
    eventBus.processAllPending();

    // Final state: Idle
    assert(controller.getState() == ExitGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 0); // Ticket used
    printInfo("State: Idle (ticket used, complete cycle)");

    printSuccess("Complete exit flow with paid ticket works correctly!");
}

/**
 * Test 3: Exit Flow Rejected - Unpaid Ticket
 * Simulates: ticket validate 1 (unpaid)
 * Expected: Validation fails, barrier stays closed
 */
void test_exit_flow_unpaid_rejected() {
    printTestHeader("Exit Flow Rejected - Unpaid Ticket");

    // Setup
    MockEventBus eventBus;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);

    // Create an UNPAID ticket
    uint32_t ticketId = ticketService.getNewTicket();
    printInfo("Ticket #1 created (UNPAID)");

    ExitGateController controller(eventBus, exitGate, ticketService, 100);

    // Initial state
    assert(controller.getState() == ExitGateState::Idle);
    printInfo("Initial state: Idle");

    // Simulate: ticket validate 1 (should fail)
    printInfo("Command: ticket validate 1 (unpaid)");
    bool validated = controller.validateTicketManually(ticketId);
    assert(validated == false);
    printInfo("Validation rejected: Ticket not paid");
    eventBus.processAllPending();

    // State should remain Idle
    assert(controller.getState() == ExitGateState::Idle);
    assert(exitGate.isOpen() == false);
    printInfo("State: Idle (barrier remains closed)");

    // Ticket should still be active
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("Ticket still active (not used)");

    printSuccess("Exit flow correctly rejects unpaid ticket!");
}

/**
 * Test 4: Entry Flow Rejected - Parking Full
 * Simulates: publish EntryButtonPressed when capacity is full
 * Expected: Entry rejected, no ticket issued
 */
void test_entry_flow_parking_full() {
    printTestHeader("Entry Flow Rejected - Parking Full");

    // Setup
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockTicketService ticketService(2); // Small capacity

    // Fill the parking (issue 2 tickets)
    ticketService.getNewTicket();
    ticketService.getNewTicket();
    assert(ticketService.getActiveTicketCount() == 2);
    printInfo("Parking full: 2/2 spaces occupied");

    EntryGateController controller(eventBus, entryButton, entryGate, ticketService, 100);

    // Initial state
    assert(controller.getState() == EntryGateState::Idle);
    printInfo("Initial state: Idle");

    // Simulate: publish EntryButtonPressed (should be rejected)
    printInfo("Command: publish EntryButtonPressed (parking full)");
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    eventBus.processAllPending();

    // State: CheckingCapacity
    assert(controller.getState() == EntryGateState::CheckingCapacity);
    printInfo("State: CheckingCapacity (checking capacity)");
    eventBus.processAllPending();

    // Should return to Idle (parking full)
    assert(controller.getState() == EntryGateState::Idle);
    assert(entryGate.isOpen() == false);
    printInfo("State: Idle (entry rejected, barrier closed)");

    // No new ticket should be issued
    assert(ticketService.getActiveTicketCount() == 2);
    printInfo("No new ticket issued");

    printSuccess("Entry flow correctly rejects when parking is full!");
}

/**
 * Test 5: Multiple Vehicles Sequential Flow
 * Simulates: 3 cars entering and exiting in sequence
 */
void test_multiple_vehicles_sequential() {
    printTestHeader("Multiple Vehicles Sequential Flow");

    // Setup
    MockEventBus eventBus;
    MockGpioInput entryButton;
    MockGpioInput entryLightBarrier;
    MockGate entryGate;
    MockGate exitGate;
    MockGpioInput exitLightBarrier;
    MockTicketService ticketService(5);

    EntryGateController entryController(eventBus, entryButton, entryGate, ticketService, 100);
    ExitGateController exitController(eventBus, exitGate, ticketService, 100);

    // Process 3 entries
    for (int i = 1; i <= 3; i++) {
        printInfo(("Car " + std::to_string(i) + " entering...").c_str());

        // Entry button press
        Event buttonEvent(EventType::EntryButtonPressed);
        eventBus.publish(buttonEvent);
        eventBus.processAllPending(); // CheckingCapacity
        eventBus.processAllPending(); // IssuingTicket
        eventBus.processAllPending(); // OpeningBarrier
        eventBus.processAllPending(); // WaitingForCar

        // Car passes
        Event blocked(EventType::EntryLightBarrierBlocked);
        eventBus.publish(blocked);
        eventBus.processAllPending(); // CarPassing

        Event cleared(EventType::EntryLightBarrierCleared);
        eventBus.publish(cleared);
        eventBus.processAllPending(); // WaitingBeforeClose
        eventBus.processAllPending(); // ClosingBarrier
        eventBus.processAllPending(); // Idle

        assert(entryController.getState() == EntryGateState::Idle);
    }

    assert(ticketService.getActiveTicketCount() == 3);
    printInfo("3 cars entered successfully");

    // Pay all tickets
    for (uint32_t id = 1; id <= 3; id++) {
        ticketService.payTicket(id);
    }
    printInfo("All tickets paid");

    // Process 3 exits
    for (uint32_t id = 1; id <= 3; id++) {
        printInfo(("Car " + std::to_string(id) + " exiting...").c_str());

        // Validate ticket
        bool validated = exitController.validateTicketManually(id);
        assert(validated == true);
        eventBus.processAllPending(); // ValidatingTicket
        eventBus.processAllPending(); // OpeningBarrier
        eventBus.processAllPending(); // WaitingForCarToPass

        // Car passes
        Event blocked(EventType::ExitLightBarrierBlocked);
        eventBus.publish(blocked);
        eventBus.processAllPending(); // CarPassing

        Event cleared(EventType::ExitLightBarrierCleared);
        eventBus.publish(cleared);
        eventBus.processAllPending(); // WaitingBeforeClose
        eventBus.processAllPending(); // ClosingBarrier
        eventBus.processAllPending(); // Idle

        assert(exitController.getState() == ExitGateState::Idle);
    }

    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("All 3 cars exited successfully");

    printSuccess("Multiple vehicles sequential flow works correctly!");
}

/**
 * Test 6: Full Workflow - Entry to Exit
 * Simulates complete workflow from entry button to exit
 */
void test_full_workflow_entry_to_exit() {
    printTestHeader("Full Workflow - Entry to Exit");

    // Setup
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

    // Step 1: Entry
    printInfo("Step 1: publish EntryButtonPressed");
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);

    // Process entry states
    eventBus.processAllPending(); // CheckingCapacity
    eventBus.processAllPending(); // IssuingTicket
    uint32_t ticketId = ticketService.getActiveTicketCount(); // Get ticket ID
    eventBus.processAllPending(); // OpeningBarrier
    eventBus.processAllPending(); // WaitingForCar

    // Car enters and clears
    Event entryBlocked(EventType::EntryLightBarrierBlocked);
    eventBus.publish(entryBlocked);
    eventBus.processAllPending(); // CarPassing

    Event entryCleared(EventType::EntryLightBarrierCleared);
    eventBus.publish(entryCleared);
    eventBus.processAllPending(); // WaitingBeforeClose
    eventBus.processAllPending(); // ClosingBarrier
    eventBus.processAllPending(); // Idle

    assert(entryController.getState() == EntryGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 1);
    printInfo("Entry complete: Ticket #1 issued");

    // Step 2: Pay ticket
    printInfo("Step 2: ticket pay 1");
    bool paid = ticketService.payTicket(ticketId);
    assert(paid == true);
    printInfo("Ticket #1 paid");

    // Step 3: Exit
    printInfo("Step 3: ticket validate 1");
    bool validated = exitController.validateTicketManually(ticketId);
    assert(validated == true);

    eventBus.processAllPending(); // ValidatingTicket
    eventBus.processAllPending(); // OpeningBarrier
    eventBus.processAllPending(); // WaitingForCarToPass

    printInfo("Step 4: publish ExitLightBarrierBlocked");
    Event exitBlocked(EventType::ExitLightBarrierBlocked);
    eventBus.publish(exitBlocked);
    eventBus.processAllPending(); // CarPassing

    Event exitCleared(EventType::ExitLightBarrierCleared);
    eventBus.publish(exitCleared);
    eventBus.processAllPending(); // WaitingBeforeClose
    eventBus.processAllPending(); // ClosingBarrier
    eventBus.processAllPending(); // Idle

    assert(exitController.getState() == ExitGateState::Idle);
    assert(ticketService.getActiveTicketCount() == 0);
    printInfo("Exit complete: Parking empty");

    printSuccess("Full workflow from entry to exit works correctly!");
}

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
