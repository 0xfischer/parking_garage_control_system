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
static int cmd_status(int argc, char** argv) {
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
static int cmd_ticket(int argc, char** argv) {
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

// Command: gpio (with subcommands: read)
static int cmd_gpio_read(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: gpio <read> <entry|exit> <button|barrier|motor>\n");
        return 1;
    }

    const char* subcommand = argv[1];

    // Subcommand: read
    if (strcmp(subcommand, "read") == 0) {
        if (argc < 4) {
            printf("Usage: gpio read <entry|exit> <button|barrier|motor>\n");
            return 1;
        }

        const char* gate = argv[2];
        const char* device = argv[3];

        // This is a simplified version - in production you'd store GPIO references
        printf("GPIO read: %s %s\n", gate, device);
        printf("(GPIO direct read not implemented in this version - use 'status' command)\n");

        return 0;
    }

    printf("Error: Unknown subcommand '%s'\n", subcommand);
    printf("Usage: gpio <read> <entry|exit> <button|barrier|motor>\n");
    return 1;
}

// Command: publish (with subcommands: list or event name)
static int cmd_publish(int argc, char** argv) {
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

// Command: help (custom)
static int cmd_help(int argc, char** argv) {
    printf("\n=== Parking Garage Control System ===\n\n");
    printf("Available Commands:\n");
    printf("  status                    - Show system status\n");
    printf("  ticket list               - List all tickets\n");
    printf("  ticket pay <id>           - Pay ticket\n");
    printf("  ticket validate <id>      - Validate ticket for exit\n");
    printf("  publish <event>           - Publish an event (use 'list' to see all)\n");
    printf("  gpio read <gate> <dev>    - Read GPIO state\n");
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

    const esp_console_cmd_t gpio_read_cmd = {
        .command = "gpio",
        .help = "GPIO operations (read)",
        .hint = nullptr,
        .func = &cmd_gpio_read,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&gpio_read_cmd);

    const esp_console_cmd_t help_cmd = {
        .command = "?",
        .help = "Show available commands",
        .hint = nullptr,
        .func = &cmd_help,
        .argtable = nullptr,
    };
    esp_console_cmd_register(&help_cmd);

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
