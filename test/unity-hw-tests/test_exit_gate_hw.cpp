/**
 * @file test_exit_gate_hw.cpp
 * @brief Hardware integration tests for Exit Gate Controller
 *
 * These tests run on real ESP32 hardware and verify the complete
 * exit gate workflow including GPIO interactions.
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ParkingGarageSystem.h"
#include "ParkingGarageConfig.h"
#include "ExitGateController.h"

static const char* TAG = "test_exit_hw";

// External test system from entry tests
extern ParkingGarageSystem* g_system;

// GPIO pins from default config
static constexpr gpio_num_t EXIT_LIGHT_BARRIER_PIN = GPIO_NUM_4;

/**
 * Test: Unpaid ticket rejection
 */
TEST_CASE("Exit rejects unpaid ticket", "[exit][hw]") {
    auto& ticketService = g_system->getTicketService();
    auto& exitController = g_system->getExitGate();

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
TEST_CASE("Exit accepts paid ticket", "[exit][hw]") {
    auto& ticketService = g_system->getTicketService();
    auto& exitController = g_system->getExitGate();

    // Wait for idle
    int timeout = 50;
    while (exitController.getState() != ExitGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }

    // Issue and pay a ticket
    uint32_t ticketId = ticketService.getNewTicket();
    TEST_ASSERT_NOT_EQUAL(0, ticketId);

    bool paid = ticketService.payTicket(ticketId);
    TEST_ASSERT_TRUE(paid);

    // Validate ticket
    bool validated = exitController.validateTicketManually(ticketId);

    TEST_ASSERT_TRUE(validated);
    TEST_ASSERT_NOT_EQUAL(ExitGateState::Idle, exitController.getState());

    ESP_LOGI(TAG, "Paid ticket validation test passed");

    // Let the exit flow complete
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Simulate car passing through exit
    gpio_set_direction(EXIT_LIGHT_BARRIER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(EXIT_LIGHT_BARRIER_PIN, 0); // Block
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(EXIT_LIGHT_BARRIER_PIN, 1); // Clear
    gpio_set_direction(EXIT_LIGHT_BARRIER_PIN, GPIO_MODE_INPUT);

    // Wait for barrier to close
    vTaskDelay(pdMS_TO_TICKS(5000));

    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());
}

/**
 * Test: Complete exit cycle
 */
TEST_CASE("Complete exit cycle with light barrier", "[exit][hw][slow]") {
    auto& ticketService = g_system->getTicketService();
    auto& exitController = g_system->getExitGate();

    // Wait for idle
    int timeout = 50;
    while (exitController.getState() != ExitGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }
    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());

    // Issue and pay a ticket
    uint32_t ticketId = ticketService.getNewTicket();
    ticketService.payTicket(ticketId);

    uint32_t activeBeforeExit = ticketService.getActiveTicketCount();

    // Validate ticket to start exit flow
    exitController.validateTicketManually(ticketId);

    // Wait for barrier to open
    vTaskDelay(pdMS_TO_TICKS(3000));

    TEST_ASSERT_EQUAL(ExitGateState::WaitingForCarToPass, exitController.getState());

    // Simulate car passing
    gpio_set_direction(EXIT_LIGHT_BARRIER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(EXIT_LIGHT_BARRIER_PIN, 0); // Car enters barrier area
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL(ExitGateState::CarPassing, exitController.getState());

    gpio_set_level(EXIT_LIGHT_BARRIER_PIN, 1); // Car clears barrier
    gpio_set_direction(EXIT_LIGHT_BARRIER_PIN, GPIO_MODE_INPUT);

    // Wait for safety delay + close
    vTaskDelay(pdMS_TO_TICKS(5000));

    TEST_ASSERT_EQUAL(ExitGateState::Idle, exitController.getState());

    // Ticket should be consumed
    uint32_t activeAfterExit = ticketService.getActiveTicketCount();
    TEST_ASSERT_EQUAL(activeBeforeExit - 1, activeAfterExit);

    ESP_LOGI(TAG, "Complete exit cycle test passed");
}
