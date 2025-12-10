/**
 * Unity Hardware Tests - Main Entry Point
 *
 * Runs Unity test framework with all registered TEST_CASEs
 */

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Import test cases from test files (linked via CMake)
// Unity's TEST_CASE macro auto-registers tests

extern "C" void app_main(void) {
    // Small delay to let serial stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n\n=== Unity Hardware Tests ===\n\n");

    // Run all tests
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    printf("\n=== Tests Complete ===\n");

    // Keep running so output can be read
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
