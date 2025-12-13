/**
 * @file test_exit_gate_hw.cpp
 * @brief Hardware integration tests for Exit Gate Controller
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
#include "ExitGateController.h"
#include "Event.h"
#include "Gate.h"
#include "EspGpioInput.h"

static const char* TAG = "test_exit_hw";

/**
 * Helper: Wait for exit gate to return to Idle state
 */
static void wait_for_exit_idle(ExitGateController& controller, int max_wait_ms = 10000) {
    int timeout = max_wait_ms / 100;
    while (controller.getState() != ExitGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }
}

/**
 * Test: Unpaid ticket rejection
 */
static void test_exit_rejects_unpaid_ticket(void) {
    auto& system = get_test_system();
    auto& ticketService = system.getTicketService();
    auto& exitController = system.getExitGate();

    wait_for_exit_idle(exitController);

    // Issue a new ticket (unpaid)
    uint32_t ticketId = ticketService.getNewTicket();
    TEST_ASSERT_NOT_EQUAL(0, ticketId);

    // Try to validate unpaid ticket
    bool validated = exitController.validateTicketManually(ticketId);

    TEST_ASSERT_FALSE(validated);
    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());

    ESP_LOGI(TAG, "Unpaid ticket rejection test passed");
}

/**
 * Test: Paid ticket allows exit
 */
static void test_exit_accepts_paid_ticket(void) {
    auto& system = get_test_system();
    auto& ticketService = system.getTicketService();
    auto& exitController = system.getExitGate();
    auto& eventBus = system.getEventBus();

    wait_for_exit_idle(exitController);

    // Issue and pay a ticket
    uint32_t ticketId = ticketService.getNewTicket();
    TEST_ASSERT_NOT_EQUAL(0, ticketId);

    bool paid = ticketService.payTicket(ticketId);
    TEST_ASSERT_TRUE(paid);

    // Validate ticket
    bool validated = exitController.validateTicketManually(ticketId);
    TEST_ASSERT_TRUE(validated);

    // Should have started exit process
    vTaskDelay(pdMS_TO_TICKS(300));
    TEST_ASSERT_NOT_EQUAL(ExitGateState::Idle, exitController.getState());

    ESP_LOGI(TAG, "Paid ticket validation test passed");

    // Complete the exit flow
    vTaskDelay(pdMS_TO_TICKS(800)); // Wait for barrier to open (500ms timeout)

    Event blockEvent(EventType::ExitLightBarrierBlocked);
    eventBus.publish(blockEvent);
    vTaskDelay(pdMS_TO_TICKS(200));

    Event clearEvent(EventType::ExitLightBarrierCleared);
    eventBus.publish(clearEvent);

    vTaskDelay(pdMS_TO_TICKS(2700)); // Wait for barrier to close (2000ms wait + 500ms close + margin)
}

/**
 * Test: Complete exit cycle
 */
static void test_complete_exit_cycle(void) {
    auto& system = get_test_system();
    auto& ticketService = system.getTicketService();
    auto& exitController = system.getExitGate();
    auto& eventBus = system.getEventBus();

    wait_for_exit_idle(exitController);
    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());

    // Issue and pay a ticket
    uint32_t ticketId = ticketService.getNewTicket();
    ticketService.payTicket(ticketId);

    uint32_t activeBeforeExit = ticketService.getActiveTicketCount();

    // Validate ticket to start exit flow
    exitController.validateTicketManually(ticketId);

    // Wait for barrier to open
    vTaskDelay(pdMS_TO_TICKS(600)); // Wait for barrier to open (500ms timeout + margin)

    TEST_ASSERT_EQUAL(ExitGateState::WaitingForCarToPass, exitController.getState());

    // Simulate car passing via EventBus
    Event blockEvent(EventType::ExitLightBarrierBlocked);
    eventBus.publish(blockEvent);
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL(ExitGateState::CarPassing, exitController.getState());

    Event clearEvent(EventType::ExitLightBarrierCleared);
    eventBus.publish(clearEvent);

    // Wait for safety delay + close
    vTaskDelay(pdMS_TO_TICKS(2700)); // Wait for barrier to close (2000ms wait + 500ms close + margin)

    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());

    // Ticket should be consumed
    uint32_t activeAfterExit = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL(activeBeforeExit - 1, activeAfterExit);

    ESP_LOGI(TAG, "Complete exit cycle test passed");
}

// ============================================================================
// GPIO-Driven Tests (using simulateInterrupt to trigger ISR directly)
// ============================================================================

/**
 * Test: GPIO light barrier triggers car detection at exit
 */
static void test_gpio_exit_light_barrier(void) {
    auto& system = get_test_system();
    auto& ticketService = system.getTicketService();
    auto& exitController = system.getExitGate();
    auto& exitGate = system.getExitGateHardware();

    wait_for_exit_idle(exitController);

    // Issue and pay a ticket
    uint32_t ticketId = ticketService.getNewTicket();
    TEST_ASSERT_NOT_EQUAL(0, ticketId);

    ticketService.payTicket(ticketId);

    // Validate ticket to start exit flow
    exitController.validateTicketManually(ticketId);
    vTaskDelay(pdMS_TO_TICKS(700)); // Wait for barrier to open (500ms timeout + processing margin)

    TEST_ASSERT_EQUAL_MESSAGE(ExitGateState::WaitingForCarToPass, exitController.getState(),
                              "Should be WaitingForCarToPass");

    exitGate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_BLOCKED);
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL_MESSAGE(ExitGateState::CarPassing, exitController.getState(),
                              "GPIO should detect car at exit");

    exitGate.getLightBarrier().simulateInterrupt(GPIO_LIGHT_BARRIER_CLEARED);
    vTaskDelay(pdMS_TO_TICKS(2700)); // Wait for barrier to close (2000ms wait + 500ms close + margin)

    TEST_ASSERT_EQUAL_MESSAGE(ExitGateState::Idle, exitController.getState(),
                              "Should return to Idle after exit");

    ESP_LOGI(TAG, "GPIO exit light barrier test passed");
}

// Register all exit gate tests
void register_exit_gate_tests(void) {
    // EventBus-driven tests
    RUN_TEST(test_exit_rejects_unpaid_ticket);
    RUN_TEST(test_exit_accepts_paid_ticket);
    RUN_TEST(test_complete_exit_cycle);
    // GPIO-driven tests
    RUN_TEST(test_gpio_exit_light_barrier);
}
