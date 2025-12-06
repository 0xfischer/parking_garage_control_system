#include "console_commands.h"
#include "esp_console.h"
#include "esp_log.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include <cstring>

static const char* TAG = "Console";
static ParkingGarageSystem* g_system = nullptr;
static esp_console_repl_t* g_repl = nullptr;

// Command: status
int cmd_status(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    char buffer[512];
    g_system->getStatus(buffer, sizeof(buffer));
    printf("%s", buffer);

    return 0;
}

// Command: ticket (with subcommands: list, pay, validate)
int cmd_ticket(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: ticket <list|pay|validate> [id]\n");
        printf("  ticket list           - List all tickets\n");
        printf("  ticket pay <id>       - Pay ticket\n");
        printf("  ticket validate <id>  - Validate ticket for exit\n");
        return 1;
    }

    const char* subcommand = argv[1];

    // Subcommand: list
    if (strcmp(subcommand, "list") == 0) {
        auto& ticketService = g_system->getTicketService();
        uint32_t active = ticketService.getActiveTicketCount();
        uint32_t capacity = ticketService.getCapacity();

        printf("=== Ticket System ===\n");
        printf("Active Tickets: %lu\n", active);
        printf("Capacity: %lu\n", capacity);
        printf("Available Spaces: %lu\n", capacity - active);

        // List all tickets (simplified - in real system would have better data structure)
        printf("\nActive Tickets:\n");
        for (uint32_t id = 1; id < 100; id++) {  // Check first 100 IDs
            Ticket ticket;
            if (ticketService.getTicketInfo(id, ticket) && !ticket.isUsed) {
                printf("  Ticket #%lu: %s\n", id, ticket.isPaid ? "PAID" : "UNPAID");
            }
        }

        return 0;
    }

    // Subcommand: pay
    if (strcmp(subcommand, "pay") == 0) {
        if (argc < 3) {
            printf("Error: Missing ticket ID\n");
            printf("Usage: ticket pay <id>\n");
            return 1;
        }

        uint32_t ticketId = atoi(argv[2]);
        auto& ticketService = g_system->getTicketService();

        if (ticketService.payTicket(ticketId)) {
            printf("Ticket #%lu paid successfully\n", ticketId);
            return 0;
        } else {
            printf("Error: Failed to pay ticket #%lu (not found?)\n", ticketId);
            return 1;
        }
    }

    // Subcommand: validate
    if (strcmp(subcommand, "validate") == 0) {
        if (argc < 3) {
            printf("Error: Missing ticket ID\n");
            printf("Usage: ticket validate <id>\n");
            return 1;
        }

        uint32_t ticketId = atoi(argv[2]);

        // Try to manually validate through exit gate
        if (g_system->getExitGate().validateTicketManually(ticketId)) {
            printf("Ticket #%lu validated successfully\n", ticketId);
            return 0;
        } else {
            printf("Error: Failed to validate ticket #%lu\n", ticketId);
            return 1;
        }
    }

    printf("Error: Unknown subcommand '%s'\n", subcommand);
    printf("Usage: ticket <list|pay|validate> [id]\n");
    return 1;
}

// Command: gpio (with subcommands: read, write)
int cmd_gpio(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: gpio <read|write> <entry|exit> <component> [value]\n");
        printf("\n");
        printf("Read commands:\n");
        printf("  gpio read entry button     - Read entry button (GPIO 25)\n");
        printf("  gpio read entry barrier    - Read entry light barrier (GPIO 23)\n");
        printf("  gpio read exit barrier     - Read exit light barrier (GPIO 4)\n");
        printf("\n");
        printf("Write commands (motor control):\n");
        printf("  gpio write entry motor <open|close>  - Control entry barrier (GPIO 22)\n");
        printf("  gpio write exit motor <open|close>   - Control exit barrier (GPIO 2)\n");
        printf("\n");
        printf("Write commands (simulation via events):\n");
        printf("  gpio write entry button pressed      - Simulate button press\n");
        printf("  gpio write entry barrier <blocked|cleared>  - Simulate light barrier\n");
        printf("  gpio write exit barrier <blocked|cleared>   - Simulate light barrier\n");
        return 1;
    }

    const char* subcommand = argv[1];

    // Subcommand: read
    if (strcmp(subcommand, "read") == 0) {
        if (argc < 4) {
            printf("Usage: gpio read <entry|exit> <button|barrier>\n");
            return 1;
        }

        const char* gate = argv[2];
        const char* component = argv[3];

        if (strcmp(gate, "entry") == 0) {
            if (strcmp(component, "button") == 0) {
                auto& entryGate = g_system->getEntryGate().getGate();
                if (entryGate.hasButton()) {
                    bool pressed = !entryGate.getButton().getLevel();  // Active low
                    printf("Entry Button (GPIO 25): %s\n", pressed ? "PRESSED" : "RELEASED");
                } else {
                    printf("Entry Button: not available\n");
                }
                return 0;
            }
            if (strcmp(component, "barrier") == 0) {
                bool blocked = g_system->getEntryGate().getGate().isCarDetected();
                printf("Entry Light Barrier (GPIO 23): %s\n", blocked ? "BLOCKED" : "CLEAR");
                return 0;
            }
            printf("Error: Unknown entry component '%s' (use: button, barrier)\n", component);
            return 1;
        }

        if (strcmp(gate, "exit") == 0) {
            if (strcmp(component, "barrier") == 0) {
                bool blocked = g_system->getExitGate().getGate().isCarDetected();
                printf("Exit Light Barrier (GPIO 4): %s\n", blocked ? "BLOCKED" : "CLEAR");
                return 0;
            }
            printf("Error: Unknown exit component '%s' (use: barrier)\n", component);
            return 1;
        }

        printf("Error: Unknown gate '%s' (use: entry, exit)\n", gate);
        return 1;
    }

    // Subcommand: write
    if (strcmp(subcommand, "write") == 0) {
        if (argc < 5) {
            printf("Usage: gpio write <entry|exit> <component> <value>\n");
            return 1;
        }

        const char* gate = argv[2];
        const char* component = argv[3];
        const char* value = argv[4];

        // Handle motor write (direct hardware control)
        if (strcmp(component, "motor") == 0) {
            bool open = false;
            if (strcmp(value, "open") == 0) {
                open = true;
            } else if (strcmp(value, "close") == 0) {
                open = false;
            } else {
                printf("Error: Unknown motor value '%s' (use: open, close)\n", value);
                return 1;
            }

            if (strcmp(gate, "entry") == 0) {
                if (open) {
                    g_system->getEntryGate().getGate().open();
                    printf("Entry Barrier (GPIO 22): OPENING\n");
                } else {
                    g_system->getEntryGate().getGate().close();
                    printf("Entry Barrier (GPIO 22): CLOSING\n");
                }
                return 0;
            }

            if (strcmp(gate, "exit") == 0) {
                if (open) {
                    g_system->getExitGate().getGate().open();
                    printf("Exit Barrier (GPIO 2): OPENING\n");
                } else {
                    g_system->getExitGate().getGate().close();
                    printf("Exit Barrier (GPIO 2): CLOSING\n");
                }
                return 0;
            }

            printf("Error: Unknown gate '%s' (use: entry, exit)\n", gate);
            return 1;
        }

        // Handle button write (simulation via event)
        if (strcmp(component, "button") == 0) {
            if (strcmp(gate, "entry") == 0) {
                if (strcmp(value, "pressed") == 0) {
                    Event event(EventType::EntryButtonPressed);
                    g_system->getEventBus().publish(event);
                    printf("Entry Button: PRESSED (event published)\n");
                    return 0;
                }
                printf("Error: Unknown button value '%s' (use: pressed)\n", value);
                return 1;
            }
            printf("Error: Only entry gate has a button\n");
            return 1;
        }

        // Handle barrier write (simulation via event)
        if (strcmp(component, "barrier") == 0) {
            if (strcmp(gate, "entry") == 0) {
                if (strcmp(value, "blocked") == 0) {
                    Event event(EventType::EntryLightBarrierBlocked);
                    g_system->getEventBus().publish(event);
                    printf("Entry Light Barrier: BLOCKED (event published)\n");
                    return 0;
                }
                if (strcmp(value, "cleared") == 0) {
                    Event event(EventType::EntryLightBarrierCleared);
                    g_system->getEventBus().publish(event);
                    printf("Entry Light Barrier: CLEARED (event published)\n");
                    return 0;
                }
                printf("Error: Unknown barrier value '%s' (use: blocked, cleared)\n", value);
                return 1;
            }

            if (strcmp(gate, "exit") == 0) {
                if (strcmp(value, "blocked") == 0) {
                    Event event(EventType::ExitLightBarrierBlocked);
                    g_system->getEventBus().publish(event);
                    printf("Exit Light Barrier: BLOCKED (event published)\n");
                    return 0;
                }
                if (strcmp(value, "cleared") == 0) {
                    Event event(EventType::ExitLightBarrierCleared);
                    g_system->getEventBus().publish(event);
                    printf("Exit Light Barrier: CLEARED (event published)\n");
                    return 0;
                }
                printf("Error: Unknown barrier value '%s' (use: blocked, cleared)\n", value);
                return 1;
            }

            printf("Error: Unknown gate '%s' (use: entry, exit)\n", gate);
            return 1;
        }

        printf("Error: Unknown component '%s' (use: motor, button, barrier)\n", component);
        return 1;
    }

    printf("Error: Unknown subcommand '%s'\n", subcommand);
    printf("Usage: gpio <read|write> <entry|exit> <component> [value]\n");
    return 1;
}

// Command: publish (with subcommands: list or event name)
int cmd_publish(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: publish <event-name|list>\n");
        printf("  publish list  - Show all available events\n");
        printf("  publish <event-name>  - Publish an event\n");
        return 1;
    }

    const char* eventName = argv[1];

    // Show list of available events
    if (strcmp(eventName, "list") == 0) {
        printf("\n=== Available Events ===\n\n");
        printf("Entry Gate Events:\n");
        printf("  EntryButtonPressed        - Simulate entry button press\n");
        printf("  EntryLightBarrierBlocked  - Block entry light barrier\n");
        printf("  EntryLightBarrierCleared  - Clear entry light barrier\n");
        printf("\n");
        printf("Exit Gate Events:\n");
        printf("  ExitLightBarrierBlocked   - Block exit light barrier\n");
        printf("  ExitLightBarrierCleared   - Clear exit light barrier\n");
        printf("\n");
        return 0;
    }

    // Publish the requested event
    EventType eventType;

    if (strcmp(eventName, "EntryButtonPressed") == 0) {
        eventType = EventType::EntryButtonPressed;
    } else if (strcmp(eventName, "EntryLightBarrierBlocked") == 0) {
        eventType = EventType::EntryLightBarrierBlocked;
    } else if (strcmp(eventName, "EntryLightBarrierCleared") == 0) {
        eventType = EventType::EntryLightBarrierCleared;
    } else if (strcmp(eventName, "ExitLightBarrierBlocked") == 0) {
        eventType = EventType::ExitLightBarrierBlocked;
    } else if (strcmp(eventName, "ExitLightBarrierCleared") == 0) {
        eventType = EventType::ExitLightBarrierCleared;
    } else {
        printf("Error: Unknown event '%s'\n", eventName);
        printf("Use 'publish list' to see available events\n");
        return 1;
    }

    printf("Publishing event: %s\n", eventName);
    Event event(eventType);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: test (hardware test workflows)
int cmd_test(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: test <entry|exit|full|info>\n");
        printf("  test entry  - Guide for entry gate hardware test\n");
        printf("  test exit   - Guide for exit gate hardware test\n");
        printf("  test full   - Guide for complete entry-to-exit test\n");
        printf("  test info   - Show GPIO pin assignments\n");
        return 1;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "info") == 0) {
        printf("\n=== Hardware Test GPIO Info ===\n\n");
        printf("Entry Gate:\n");
        printf("  Button:        GPIO 25 (pull LOW to press)\n");
        printf("  Light Barrier: GPIO 23 (pull LOW to block)\n");
        printf("  Servo Motor:   GPIO 22 (PWM output)\n");
        printf("\n");
        printf("Exit Gate:\n");
        printf("  Light Barrier: GPIO 4 (pull LOW to block)\n");
        printf("  Servo Motor:   GPIO 2 (PWM output)\n");
        printf("\n");
        return 0;
    }

    if (strcmp(subcommand, "entry") == 0) {
        printf("\n=== Entry Gate Hardware Test ===\n\n");
        printf("Current State: %s\n\n",
            g_system->getEntryGate().getState() == EntryGateState::Idle ? "Idle" : "Active");

        printf("Test Steps:\n");
        printf("1. Press entry button (GPIO 25 -> GND)\n");
        printf("   Expected: Ticket issued, barrier opens\n");
        printf("\n");
        printf("2. Wait for barrier to open (~2 sec)\n");
        printf("   Expected: State = WaitingForCar\n");
        printf("\n");
        printf("3. Block light barrier (GPIO 23 -> GND)\n");
        printf("   Expected: State = CarPassing\n");
        printf("\n");
        printf("4. Clear light barrier (GPIO 23 release)\n");
        printf("   Expected: State = WaitingBeforeClose (2 sec)\n");
        printf("\n");
        printf("5. Wait for barrier to close\n");
        printf("   Expected: State = Idle\n");
        printf("\n");
        printf("Use 'status' to check current state.\n");
        return 0;
    }

    if (strcmp(subcommand, "exit") == 0) {
        printf("\n=== Exit Gate Hardware Test ===\n\n");
        printf("Current State: %s\n\n",
            g_system->getExitGate().getState() == ExitGateState::Idle ? "Idle" : "Active");

        printf("Prerequisites:\n");
        printf("- At least one paid ticket (use 'ticket pay <id>')\n");
        printf("\n");
        printf("Test Steps:\n");
        printf("1. Validate ticket: ticket validate <id>\n");
        printf("   Expected: Barrier opens\n");
        printf("\n");
        printf("2. Wait for barrier to open (~2 sec)\n");
        printf("   Expected: State = WaitingForCarToPass\n");
        printf("\n");
        printf("3. Block light barrier (GPIO 4 -> GND)\n");
        printf("   Expected: State = CarPassing\n");
        printf("\n");
        printf("4. Clear light barrier (GPIO 4 release)\n");
        printf("   Expected: State = WaitingBeforeClose (2 sec)\n");
        printf("\n");
        printf("5. Wait for barrier to close\n");
        printf("   Expected: State = Idle, ticket consumed\n");
        printf("\n");
        return 0;
    }

    if (strcmp(subcommand, "full") == 0) {
        printf("\n=== Full Workflow Hardware Test ===\n\n");
        printf("This test simulates a complete parking session.\n\n");

        printf("=== ENTRY ===\n");
        printf("1. Press entry button (GPIO 25)\n");
        printf("2. Wait for barrier, block/clear light barrier (GPIO 23)\n");
        printf("3. Check: 'ticket list' shows new unpaid ticket\n");
        printf("\n");

        printf("=== PAYMENT ===\n");
        printf("4. Pay ticket: 'ticket pay 1'\n");
        printf("5. Check: 'ticket list' shows PAID status\n");
        printf("\n");

        printf("=== EXIT ===\n");
        printf("6. Validate: 'ticket validate 1'\n");
        printf("7. Wait for barrier, block/clear light barrier (GPIO 4)\n");
        printf("8. Check: 'ticket list' shows no active tickets\n");
        printf("9. Check: 'status' shows parking space freed\n");
        printf("\n");
        return 0;
    }

    printf("Error: Unknown subcommand '%s'\n", subcommand);
    printf("Usage: test <entry|exit|full|info>\n");
    return 1;
}

// Command: help (custom)
int cmd_help(int argc, char** argv) {
    printf("\n=== Parking Garage Control System ===\n\n");
    printf("Available Commands:\n");
    printf("  status                    - Show system status\n");
    printf("  ticket list               - List all tickets\n");
    printf("  ticket pay <id>           - Pay ticket\n");
    printf("  ticket validate <id>      - Validate ticket for exit\n");
    printf("  publish <event>           - Publish event (use 'list')\n");
    printf("  gpio                      - GPIO read/write (use for usage)\n");
    printf("  test <entry|exit|full|info>  - Hardware test guides\n");
    printf("  ?                         - Show this help\n");
    printf("  help                      - Show ESP-IDF help\n");
    printf("  clear                     - Clear screen\n");
    printf("  restart                   - Restart system\n");
    printf("\n");
    return 0;
}

void console_init(ParkingGarageSystem* system) {
    g_system = system;

    // Register built-in help command first
    esp_console_register_help_command();

    // Register commands
    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Show system status",
        .hint = nullptr,
        .func = &cmd_status,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&status_cmd);

    const esp_console_cmd_t ticket_cmd = {
        .command = "ticket",
        .help = "Ticket management (list|pay|validate)",
        .hint = nullptr,
        .func = &cmd_ticket,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&ticket_cmd);

    const esp_console_cmd_t publish_cmd = {
        .command = "publish",
        .help = "Publish an event (use 'list' to see all)",
        .hint = nullptr,
        .func = &cmd_publish,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&publish_cmd);

    const esp_console_cmd_t gpio_cmd = {
        .command = "gpio",
        .help = "GPIO read/write (use 'gpio' for usage)",
        .hint = nullptr,
        .func = &cmd_gpio,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&gpio_cmd);

    const esp_console_cmd_t help_cmd = {
        .command = "?",
        .help = "Show available commands",
        .hint = nullptr,
        .func = &cmd_help,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&help_cmd);

    const esp_console_cmd_t test_cmd = {
        .command = "test",
        .help = "Hardware test guides (entry|exit|full|info)",
        .hint = nullptr,
        .func = &cmd_test,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&test_cmd);

    ESP_LOGI(TAG, "Console commands registered");
}

void console_start() {
    // Configure and start REPL
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "ParkingGarage> ";
    repl_config.max_cmdline_length = 256;

    // Configure UART for console

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    // Create UART REPL
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &g_repl));

    // Start REPL
    ESP_LOGI(TAG, "Starting console REPL...");
    ESP_ERROR_CHECK(esp_console_start_repl(g_repl));

    ESP_LOGI(TAG, "Console ready. Type '?' for help.");
}
