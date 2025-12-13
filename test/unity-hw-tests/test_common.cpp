/**
 * @file test_common.cpp
 * @brief Common test infrastructure implementation
 */

#include "test_common.h"
#include "esp_log.h"

static const char* TAG = "test_common";

// Static system instance - lazy initialized
static ParkingGarageSystem* s_system = nullptr;

/**
 * @brief Create test configuration with fast timeouts
 *
 * Uses shorter timeouts for faster test execution:
 * - barrierTimeoutMs: 500ms (instead of 2000ms)
 * - capacity: 2 (small to test "parking full" scenario)
 */
static ParkingGarageConfig createTestConfig() {
    ParkingGarageConfig config = ParkingGarageConfig::fromKconfig();

    // Override for faster tests
    config.barrierTimeoutMs = 500; // 500ms instead of 2000ms
    config.capacity = 2;           // Small capacity to test "parking full"

    ESP_LOGI(TAG, "Test config: barrierTimeout=%lums, capacity=%lu",
             config.barrierTimeoutMs, config.capacity);

    return config;
}

ParkingGarageSystem& get_test_system() {
    if (s_system == nullptr) {
        ESP_LOGI(TAG, "Initializing test system...");

        ParkingGarageConfig config = createTestConfig();
        s_system = new ParkingGarageSystem(config);
        s_system->initialize();

        // Start event loop
        auto& eventBus = static_cast<FreeRtosEventBus&>(s_system->getEventBus());
        eventBus.startEventLoop();

        // Give event loop time to start
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "Test system ready");
    }
    return *s_system;
}

void reset_test_system() {
    if (s_system != nullptr) {
        s_system->reset();
    }
}

// Unity setUp - called before each test
extern "C" void setUp(void) {
    reset_test_system();
}

// Unity tearDown - called after each test
extern "C" void tearDown(void) {
    // Currently empty, but required by Unity
}
