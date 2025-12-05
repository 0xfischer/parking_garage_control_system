#pragma once

#include "ParkingGarageSystem.h"

/**
 * @brief Initialize and register console commands
 * @param system Parking garage system instance
 */
void console_init(ParkingGarageSystem* system);

/**
 * @brief Start console REPL (Read-Eval-Print Loop)
 */
void console_start();
