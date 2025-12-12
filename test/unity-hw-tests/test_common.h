/**
 * @file test_common.h
 * @brief Common test infrastructure for Unity hardware tests
 *
 * Provides shared system initialization and helper functions
 * for all hardware integration tests.
 */

#pragma once

#include "ParkingGarageSystem.h"
#include "ParkingGarageConfig.h"
#include "FreeRtosEventBus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Get the shared test system instance
 *
 * Initializes the parking system on first call (lazy initialization).
 * Subsequent calls return the same instance.
 */
ParkingGarageSystem& get_test_system();

/**
 * @brief Reset test system to initial state
 *
 * Called by setUp() before each test to ensure consistent starting conditions.
 * Resets all controllers and clears all tickets.
 */
void reset_test_system();

// =============================================================================
// GPIO Simulation Constants
// =============================================================================
// These constants define the GPIO levels for simulateInterrupt() calls.
// The mapping is defined in ParkingGarageSystem::initialize():
//   - Light barrier: LOW=blocked (car detected), HIGH=cleared (car passed)
//   - Button: Active low - LOW=pressed, HIGH=released

/** @brief GPIO level to simulate light barrier blocked (car detected) */
constexpr bool GPIO_LIGHT_BARRIER_BLOCKED = false;  // LOW = car detected

/** @brief GPIO level to simulate light barrier cleared (car passed) */
constexpr bool GPIO_LIGHT_BARRIER_CLEARED = true;   // HIGH = no car

/** @brief GPIO level to simulate button pressed (active low) */
constexpr bool GPIO_BUTTON_PRESSED = false;         // LOW = pressed

/** @brief GPIO level to simulate button released (active low) */
constexpr bool GPIO_BUTTON_RELEASED = true;         // HIGH = released
