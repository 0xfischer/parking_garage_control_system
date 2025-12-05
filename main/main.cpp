#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "ParkingGarageSystem.h"
#include "ParkingGarageConfig.h"
#include "console_commands.h"

static const char* TAG = "Main";

// Global parking garage system instance
static ParkingGarageSystem* g_parkingSystem = nullptr;

/**
 * @brief Main event loop task
 *
 * Continuously processes events from the event bus.
 * This is the heart of the event-driven architecture.
 */
static void event_loop_task(void* pvParameters) {
    auto* system = static_cast<ParkingGarageSystem*>(pvParameters);
    IEventBus& eventBus = system->getEventBus();

    ESP_LOGI(TAG, "Event loop task started");

    Event event;
    while (true) {
        // Wait for next event (blocking)
        if (eventBus.waitForEvent(event, portMAX_DELAY)) {
            // Event is automatically dispatched to subscribers in waitForEvent
            // Just log for debugging
            ESP_LOGD(TAG, "Event processed: %s", eventTypeToString(event.type));
        }
    }
}

/**
 * @brief Initialize NVS (Non-Volatile Storage)
 */
static void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Parking Garage Control System");
    ESP_LOGI(TAG, "  Event-Driven Architecture");
    ESP_LOGI(TAG, "  ESP32 + FreeRTOS + C++20");
    ESP_LOGI(TAG, "========================================");

    // Initialize NVS
    init_nvs();

    // Get configuration from Kconfig
    ParkingGarageConfig config = ParkingGarageConfig::fromKconfig();

    // Create parking garage system
    ESP_LOGI(TAG, "Creating parking garage system...");
    g_parkingSystem = new ParkingGarageSystem(config);

    // Initialize system (setup GPIO interrupts, etc.)
    ESP_LOGI(TAG, "Initializing parking garage system...");
    g_parkingSystem->initialize();

    // Create event loop task
    ESP_LOGI(TAG, "Starting event loop task...");
    xTaskCreate(
        event_loop_task,
        "event_loop",
        4096,  // Stack size
        g_parkingSystem,
        5,  // Priority (normal)
        nullptr
    );

#ifdef CONFIG_PARKING_CONSOLE_ENABLED
    // Initialize console commands
    ESP_LOGI(TAG, "Initializing console...");
    console_init(g_parkingSystem);
    console_start();

    ESP_LOGI(TAG, "System ready! Type '?' for help.");
#else
    ESP_LOGI(TAG, "System ready! Console disabled.");
#endif

    // Print initial status
    char status[512];
    g_parkingSystem->getStatus(status, sizeof(status));
    ESP_LOGI(TAG, "\n%s", status);

    // Main task can now idle or do other work
    ESP_LOGI(TAG, "Main initialization complete");

    // Keep main task alive (optional - could delete task here)
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        // Periodic status log (every 10 seconds)
        // g_parkingSystem->getStatus(status, sizeof(status));
        // ESP_LOGI(TAG, "\n%s", status);
    }
}
