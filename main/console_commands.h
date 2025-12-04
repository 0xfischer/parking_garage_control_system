#pragma once

#include "ParkingSystem.h"

/**
 * @brief Initialize and register console commands
 * @param system Parking system instance
 */
void console_init(ParkingSystem* system);

/**
 * @brief Start console REPL (Read-Eval-Print Loop)
 */
void console_start();
