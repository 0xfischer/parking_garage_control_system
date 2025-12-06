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

// Command handlers - callable directly for testing
int cmd_status(int argc, char** argv);
int cmd_ticket(int argc, char** argv);
int cmd_publish(int argc, char** argv);
int cmd_gpio(int argc, char** argv);
int cmd_test(int argc, char** argv);
int cmd_help(int argc, char** argv);
