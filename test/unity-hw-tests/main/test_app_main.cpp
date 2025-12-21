/**
 * Unity Hardware Tests - Main Entry Point
 *
 * Runs Unity test framework with registered tests.
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "nvs_flash.h"

#include "esp_log.h"

#include "../test_common.h"

#ifdef CONFIG_APPTRACE_GCOV_ENABLE
#include "esp_app_trace.h"
#endif

static const char* TAG = "test_main";

// Forward declarations - tests register themselves
extern void register_entry_gate_tests(void);
extern void register_exit_gate_tests(void);
extern void gcov_dump_to_console(void); // Custom dumper


extern "C" void app_main(void) {
    // Small delay to let serial stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n\n=== Unity Hardware Tests ===\n\n");

    // Initialize NVS (disabled to save IRAM)
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     nvs_flash_erase();
    //     nvs_flash_init();
    // }


    esp_log_level_set("*", ESP_LOG_INFO);

    // Initialize test system (creates ParkingGarageSystem + starts EventLoop)
    ESP_LOGI(TAG, "Initializing test system...");
    get_test_system();

    // Run all tests
    ESP_LOGI(TAG, "Running tests...");
    UNITY_BEGIN();
    // unity_run_tests_by_tag("[exit_gate]");
    // unity_run_all_tests();
    // unity_run_tests();
    // unity_run_menu();
    register_entry_gate_tests();
    register_exit_gate_tests();
    UNITY_END();

    printf("\n=== Tests Complete ===\n");

#ifdef CONFIG_APPTRACE_GCOV_ENABLE
    ESP_LOGI(TAG, "Dumping GCOV data...");
    esp_gcov_dump();
    ESP_LOGI(TAG, "GCOV dump complete");
#else
    // Custom manual dump to console
    ESP_LOGI(TAG, "Dumping GCOV data manually...");
    gcov_dump_to_console();
    ESP_LOGI(TAG, "GCOV dump complete");
#endif


    // Keep running so output can be read
    for (int i = 0; i < 5; ++i) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
