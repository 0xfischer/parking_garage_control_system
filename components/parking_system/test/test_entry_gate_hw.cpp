/**
 * @file test_entry_gate_hw.cpp
 * @brief Hardware integration tests for Entry Gate Controller
 *
 * These tests run on real ESP32 hardware and verify the complete
 * entry gate workflow including GPIO interactions.
 *
 * Run with: idf.py -T parking_system build flash monitor
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ParkingGarageSystem.h"
#include "ParkingGarageConfig.h"
#include "EntryGateController.h"

static const char* TAG = "test_entry_hw";

// Test system instance
static ParkingGarageSystem* g_system = nullptr;

// GPIO pins from default config
static constexpr gpio_num_t ENTRY_BUTTON_PIN = GPIO_NUM_25;
static constexpr gpio_num_t ENTRY_LIGHT_BARRIER_PIN = GPIO_NUM_23;

void setUp(void)
{
    if (g_system == nullptr) {
        ParkingGarageConfig config = ParkingGarageConfig::getDefault();
        g_system = new ParkingGarageSystem(config);
        g_system->initialize();
        // Give system time to start event loop
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void tearDown(void)
{
    // Don't delete system between tests to avoid re-initialization issues
}

/**
 * Test: Entry button press triggers state change
 */
TEST_CASE("Entry button triggers CheckingCapacity state", "[entry][hw]")
{
    auto& controller = g_system->getEntryGate();

    // Ensure we start in Idle
    TEST_ASSERT_EQUAL(EntryGateState::Idle, controller.getState());

    // Simulate button press by pulling GPIO low (active low with pull-up)
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENTRY_BUTTON_PIN, 0);  // Press
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ENTRY_BUTTON_PIN, 1);  // Release
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_INPUT);

    // Wait for state machine to process
    vTaskDelay(pdMS_TO_TICKS(200));

    // Should have moved past Idle
    TEST_ASSERT_NOT_EQUAL(EntryGateState::Idle, controller.getState());

    ESP_LOGI(TAG, "Entry button test passed - state changed");
}

/**
 * Test: Ticket issuance on entry
 */
TEST_CASE("Entry flow issues ticket", "[entry][hw]")
{
    auto& ticketService = g_system->getTicketService();
    uint32_t ticketsBefore = ticketService.getActiveTicketCount();

    // Trigger entry button
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENTRY_BUTTON_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ENTRY_BUTTON_PIN, 1);
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_INPUT);

    // Wait for ticket to be issued (goes through CheckingCapacity -> IssuingTicket)
    vTaskDelay(pdMS_TO_TICKS(500));

    uint32_t ticketsAfter = ticketService.getActiveTicketCount();

    TEST_ASSERT_EQUAL(ticketsBefore + 1, ticketsAfter);
    ESP_LOGI(TAG, "Ticket issued: count %lu -> %lu", ticketsBefore, ticketsAfter);
}

/**
 * Test: Complete entry cycle with light barrier
 */
TEST_CASE("Complete entry cycle", "[entry][hw][slow]")
{
    auto& controller = g_system->getEntryGate();

    // Start in Idle
    // Note: May need to wait for previous test to complete
    int timeout = 50;
    while (controller.getState() != EntryGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }
    TEST_ASSERT_EQUAL(EntryGateState::Idle, controller.getState());

    // Press entry button
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENTRY_BUTTON_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ENTRY_BUTTON_PIN, 1);
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_INPUT);

    // Wait for barrier to open
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Should be waiting for car
    TEST_ASSERT_EQUAL(EntryGateState::WaitingForCar, controller.getState());

    // Simulate car blocking light barrier (active low)
    gpio_set_direction(ENTRY_LIGHT_BARRIER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENTRY_LIGHT_BARRIER_PIN, 0);  // Block
    vTaskDelay(pdMS_TO_TICKS(200));

    TEST_ASSERT_EQUAL(EntryGateState::CarPassing, controller.getState());

    // Car clears barrier
    gpio_set_level(ENTRY_LIGHT_BARRIER_PIN, 1);  // Clear
    gpio_set_direction(ENTRY_LIGHT_BARRIER_PIN, GPIO_MODE_INPUT);

    // Wait for safety delay (2 sec) + barrier close
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Should return to Idle
    TEST_ASSERT_EQUAL(EntryGateState::Idle, controller.getState());

    ESP_LOGI(TAG, "Complete entry cycle test passed");
}

/**
 * Test: Parking full rejection
 */
TEST_CASE("Parking full rejects entry", "[entry][hw][capacity]")
{
    auto& ticketService = g_system->getTicketService();
    auto& controller = g_system->getEntryGate();

    // Fill parking to capacity by issuing tickets directly
    uint32_t capacity = ticketService.getCapacity();
    uint32_t current = ticketService.getActiveTicketCount();

    while (ticketService.getActiveTicketCount() < capacity) {
        ticketService.getNewTicket();
    }

    ESP_LOGI(TAG, "Parking filled: %lu/%lu", ticketService.getActiveTicketCount(), capacity);

    // Wait for idle state
    int timeout = 50;
    while (controller.getState() != EntryGateState::Idle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }

    // Try to enter
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ENTRY_BUTTON_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ENTRY_BUTTON_PIN, 1);
    gpio_set_direction(ENTRY_BUTTON_PIN, GPIO_MODE_INPUT);

    // Wait for processing
    vTaskDelay(pdMS_TO_TICKS(500));

    // Should return to Idle (entry rejected)
    TEST_ASSERT_EQUAL(EntryGateState::Idle, controller.getState());

    // Ticket count should not have increased
    TEST_ASSERT_EQUAL(capacity, ticketService.getActiveTicketCount());

    ESP_LOGI(TAG, "Parking full rejection test passed");
}
