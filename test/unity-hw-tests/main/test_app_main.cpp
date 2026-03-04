/**
 * Unity Hardware Tests - Main Entry Point
 *
 * Runs Unity test framework with registered tests.
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#ifdef ENABLE_GCOV_COVERAGE
#include "esp_gcov.h"
#endif

#include "../test_common.h"

static const char* TAG = "test_main";

// Forward declarations - tests register themselves
extern void register_entry_gate_tests(void);
extern void register_exit_gate_tests(void);

extern "C" void app_main(void) {
    // Small delay to let serial stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n\n=== Unity Hardware Tests ===\n\n");

    // Initialize NVS (required for some components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

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

#ifdef ENABLE_GCOV_COVERAGE
    printf("\n=== Dumping GCOV Coverage Data ===\n");
    printf("NOTE: This requires OpenOCD connection. In QEMU without OpenOCD,\n");
    printf("      coverage data cannot be collected. Use host tests instead:\n");
    printf("      make coverage-run\n");
    fflush(stdout);
    // esp_gcov_dump() will block waiting for OpenOCD - skip in QEMU
    // esp_gcov_dump();
    printf("=== Coverage Dump Skipped (no OpenOCD in QEMU) ===\n");
#endif

    printf("\n=== Tests Complete ===\n");
    // QEMU test completion marker - used by run_qemu_tests.sh to detect end
    printf("\n[QEMU_TEST_DONE]\n");
    fflush(stdout);

    // Keep running so output can be read
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
