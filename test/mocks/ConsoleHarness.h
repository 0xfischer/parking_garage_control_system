#pragma once

#include <string>

// Forward declarations
class ParkingGarageSystem;

// Initialize console for tests (register commands without UART/REPL)
void console_test_init(ParkingGarageSystem* system);

// Run a console command line (e.g., "publish EntryButtonPressed")
// Returns 0 on success, non-zero on error
int run_console_command(const std::string& cmdline);
