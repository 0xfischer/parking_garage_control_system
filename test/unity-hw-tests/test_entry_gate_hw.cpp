/**
 * @file test_entry_gate_hw.cpp
 * @brief Hardware integration tests for Entry Gate Controller
 *
 * These tests run on ESP32 in Wokwi simulation or real hardware.
 * Tests use both:
 * - EventBus to simulate hardware events (portable tests)
 * - simulateInterrupt() to trigger GPIO ISR handlers directly (GPIO tests)
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "test_common.h"
#include "EntryGateController.h"
#include "Event.h"
#include "Gate.h"
#include "EspGpioInput.h"

static const char* TAG = "test_entry_hw";

/**
 * Helper: Wait for entry gate to return to Idle state
 */
static void wait_for_idle(EntryGateController& controller, int max_wait_ms = 15000) {
    int timeout = max_wait_ms / 100;
    while (controller.getState() != EntryGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }
}

/**
 * Helper: Complete an entry cycle by simulating car passing through
 */
static void complete_entry_cycle(IEventBus& eventBus, EntryGateController& controller) {
    if (controller.getState() == EntryGateState::WaitingForCar) {
        Event blockEvent(EventType::EntryLightBarrierBlocked);
        eventBus.publish(blockEvent);
        vTaskDelay(pdMS_TO_TICKS(200));

        Event clearEvent(EventType::EntryLightBarrierCleared);
        eventBus.publish(clearEvent);
    }
    wait_for_idle(controller);
}

/**
 * Test: Complete entry cycle from button press to barrier close
 */
static void test_complete_entry_cycle(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& eventBus = system.getEventBus();
    auto& ticketService = system.getTicketService();

    wait_for_idle(controller);
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(), "Should start in Idle");

    uint32_t ticketsBefore = ticketService.getActiveTicketCount();

    // Simulate button press via EventBus
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);

    vTaskDelay(pdMS_TO_TICKS(600)); // Wait for barrier to open (500ms timeout + margin)

    uint32_t ticketsAfter = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL_MESSAGE(ticketsBefore + 1, ticketsAfter, "Ticket should be issued");
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::WaitingForCar, controller.getState(), "Should be WaitingForCar");

    // Simulate car passing via EventBus
    Event blockEvent(EventType::EntryLightBarrierBlocked);
    eventBus.publish(blockEvent);
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::CarPassing, controller.getState(), "Should be CarPassing");

    Event clearEvent(EventType::EntryLightBarrierCleared);
    eventBus.publish(clearEvent);

    vTaskDelay(pdMS_TO_TICKS(2700)); // Wait for barrier to close (2000ms wait + 500ms close + margin)
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(), "Should return to Idle");

    ESP_LOGI(TAG, "Complete entry cycle test passed");
}

/**
 * Test: Second entry cycle to verify multiple entries work
 */
static void test_second_entry_cycle(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& eventBus = system.getEventBus();
    auto& ticketService = system.getTicketService();

    wait_for_idle(controller);
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(), "Should start in Idle");

    uint32_t ticketsBefore = ticketService.getActiveTicketCount();

    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);

    vTaskDelay(pdMS_TO_TICKS(600)); // Wait for ticket to be issued

    uint32_t ticketsAfter = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL_MESSAGE(ticketsBefore + 1, ticketsAfter, "Second ticket should be issued");

    ESP_LOGI(TAG, "Second entry cycle test passed - tickets: %lu -> %lu", ticketsBefore, ticketsAfter);

    complete_entry_cycle(eventBus, controller);
}

/**
 * Test: Button press is ignored when not in Idle state
 */
static void test_button_ignored_when_busy(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& eventBus = system.getEventBus();
    auto& ticketService = system.getTicketService();

    wait_for_idle(controller);

    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    vTaskDelay(pdMS_TO_TICKS(300));

    TEST_ASSERT_NOT_EQUAL(EntryGateState::Idle, controller.getState());

    uint32_t ticketsBefore = ticketService.getActiveTicketCount();

    // Second button press should be ignored
    Event secondButton(EventType::EntryButtonPressed);
    eventBus.publish(secondButton);
    vTaskDelay(pdMS_TO_TICKS(300));

    uint32_t ticketsAfter = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL_MESSAGE(ticketsBefore, ticketsAfter, "Second button press should be ignored");

    ESP_LOGI(TAG, "Button ignored when busy test passed");

    complete_entry_cycle(eventBus, controller);
}

// ============================================================================
// GPIO-Driven Tests (using simulateInterrupt to trigger ISR directly)
// ============================================================================

/**
 * Helper: Complete an entry cycle using GPIO simulation
 */
static void complete_entry_cycle_gpio(Gate& gate, EntryGateController& controller) {
    if (controller.getState() == EntryGateState::WaitingForCar) {
        gate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_BLOCKED);
        vTaskDelay(pdMS_TO_TICKS(200));
        gate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_CLEARED);
    }
    wait_for_idle(controller);
}

/**
 * Test: GPIO button press triggers entry cycle
 */
static void test_gpio_button_triggers_entry(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& gate = system.getEntryGateHardware();
    auto& ticketService = system.getTicketService();

    wait_for_idle(controller);
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(), "Should start in Idle");

    uint32_t ticketsBefore = ticketService.getActiveTicketCount();

    gate.getButton().simulateInterrupt(GPIO_BUTTON_PRESSED);
    vTaskDelay(pdMS_TO_TICKS(600)); // Wait for barrier to open (500ms timeout + margin)

    uint32_t ticketsAfter = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL_MESSAGE(ticketsBefore + 1, ticketsAfter, "GPIO button should issue ticket");
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::WaitingForCar, controller.getState(), "Should be WaitingForCar");

    ESP_LOGI(TAG, "GPIO button triggers entry test passed");

    // Complete the cycle
    complete_entry_cycle_gpio(gate, controller);
}

/**
 * Test: GPIO light barrier triggers car detection
 */
static void test_gpio_light_barrier_detects_car(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& gate = system.getEntryGateHardware();
    auto& eventBus = system.getEventBus();

    wait_for_idle(controller);

    // Start entry cycle via EventBus
    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    vTaskDelay(pdMS_TO_TICKS(700)); // Wait for barrier to open (500ms timeout + processing margin)

    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::WaitingForCar, controller.getState(), "Should be WaitingForCar");

    gate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_BLOCKED);
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::CarPassing, controller.getState(), "GPIO should detect car");

    gate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_CLEARED);
    vTaskDelay(pdMS_TO_TICKS(2700)); // Wait for barrier to close (2000ms wait + 500ms close + margin)

    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(), "Should return to Idle");

    ESP_LOGI(TAG, "GPIO light barrier detects car test passed");
}

/**
 * Test: Parking full - entry rejected when capacity reached
 *
 * This test fills up the parking (capacity=3) and verifies that
 * additional entry attempts are rejected.
 */
static void test_parking_full_rejects_entry(void) {
    auto& system = get_test_system();
    auto& controller = system.getEntryGate();
    auto& eventBus = system.getEventBus();
    auto& ticketService = system.getTicketService();

    wait_for_idle(controller);

    // Get current ticket count
    uint32_t initialCount = ticketService.getActiveTicketCount();
    uint32_t capacity = ticketService.getCapacity();

    ESP_LOGI(TAG, "Parking full test: initial=%lu, capacity=%lu", initialCount, capacity);

    // Fill up remaining capacity by issuing tickets directly
    // (faster than going through full entry cycles)
    uint32_t ticketsToIssue = capacity - initialCount;
    for (uint32_t i = 0; i < ticketsToIssue; i++) {
        uint32_t ticketId = ticketService.getNewTicket();
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ticketId, "Should be able to issue ticket");
    }

    // Verify parking is now full
    TEST_ASSERT_EQUAL_MESSAGE(capacity, ticketService.getActiveTicketCount(), "Parking should be full");

    // Try to get one more ticket - should fail
    uint32_t rejectedTicket = ticketService.getNewTicket();
    TEST_ASSERT_EQUAL_MESSAGE(0, rejectedTicket, "Should reject ticket when full");

    // Try button press - should not issue ticket (capacity check fails)
    uint32_t countBefore = ticketService.getActiveTicketCount();

    Event buttonEvent(EventType::EntryButtonPressed);
    eventBus.publish(buttonEvent);
    vTaskDelay(pdMS_TO_TICKS(500));

    uint32_t countAfter = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL_MESSAGE(countBefore, countAfter, "No ticket should be issued when parking full");

    // Controller should return to Idle (entry rejected)
    wait_for_idle(controller, 2000);
    TEST_ASSERT_EQUAL_MESSAGE(EntryGateState::Idle, controller.getState(),
                              "Should return to Idle after rejection");

    ESP_LOGI(TAG, "Parking full test passed - capacity=%lu, active=%lu",
             capacity, ticketService.getActiveTicketCount());
}

// Register all entry gate tests
void register_entry_gate_tests(void) {
    // EventBus-driven tests
    RUN_TEST(test_complete_entry_cycle);
    RUN_TEST(test_second_entry_cycle);
    RUN_TEST(test_button_ignored_when_busy);
    // GPIO-driven tests
    RUN_TEST(test_gpio_button_triggers_entry);
    RUN_TEST(test_gpio_light_barrier_detects_car);
    // Capacity test (must run last as it fills parking)
    RUN_TEST(test_parking_full_rejects_entry);
}
