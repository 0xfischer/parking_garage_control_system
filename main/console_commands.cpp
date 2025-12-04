#include "console_commands.h"
#include "esp_console.h"
#include "esp_log.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include <cstring>

static const char* TAG = "Console";
static ParkingSystem* g_system = nullptr;
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

// Command: ticket list
static int cmd_ticket_list(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

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

// Command: ticket pay <id>
static struct {
    struct arg_int* ticket_id;
    struct arg_end* end;
} ticket_pay_args;

static int cmd_ticket_pay(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&ticket_pay_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ticket_pay_args.end, argv[0]);
        return 1;
    }

    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    uint32_t ticketId = ticket_pay_args.ticket_id->ival[0];
    auto& ticketService = g_system->getTicketService();

    if (ticketService.payTicket(ticketId)) {
        printf("Ticket #%lu paid successfully\n", ticketId);
        return 0;
    } else {
        printf("Error: Failed to pay ticket #%lu (not found?)\n", ticketId);
        return 1;
    }
}

// Command: ticket validate <id>
static struct {
    struct arg_int* ticket_id;
    struct arg_end* end;
} ticket_validate_args;

static int cmd_ticket_validate(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&ticket_validate_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ticket_validate_args.end, argv[0]);
        return 1;
    }

    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    uint32_t ticketId = ticket_validate_args.ticket_id->ival[0];

    // Try to manually validate through exit gate
    if (g_system->getExitGate().validateTicketManually(ticketId)) {
        printf("Ticket #%lu validated successfully\n", ticketId);
        return 0;
    } else {
        printf("Error: Failed to validate ticket #%lu\n", ticketId);
        return 1;
    }
}

// Command: gpio read <entry|exit> <button|barrier|motor>
static int cmd_gpio_read(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: gpio read <entry|exit> <button|barrier|motor>\n");
        return 1;
    }

    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    // Parse arguments manually
    const char* gate = argv[1];
    const char* device = argv[2];

    // This is a simplified version - in production you'd store GPIO references
    printf("GPIO read: %s %s\n", gate, device);
    printf("(GPIO direct read not implemented in this version - use 'status' command)\n");

    return 0;
}

// Command: simulate entry
static int cmd_simulate_entry(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Simulating entry button press...\n");
    Event event(EventType::EntryButtonPressed);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: simulate exit
static int cmd_simulate_exit(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Simulating car at exit...\n");
    Event event(EventType::ExitLightBarrierBlocked);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: entry barrier block
static int cmd_entry_barrier_block(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Entry light barrier: BLOCKED\n");
    Event event(EventType::EntryLightBarrierBlocked);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: entry barrier clear
static int cmd_entry_barrier_clear(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Entry light barrier: CLEAR\n");
    Event event(EventType::EntryLightBarrierCleared);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: exit barrier block
static int cmd_exit_barrier_block(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Exit light barrier: BLOCKED\n");
    Event event(EventType::ExitLightBarrierBlocked);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: exit barrier clear
static int cmd_exit_barrier_clear(int argc, char** argv) {
    if (!g_system) {
        printf("Error: System not initialized\n");
        return 1;
    }

    printf("Exit light barrier: CLEAR\n");
    Event event(EventType::ExitLightBarrierCleared);
    g_system->getEventBus().publish(event);

    return 0;
}

// Command: help (custom)
static int cmd_help(int argc, char** argv) {
    printf("\n=== Parking Garage Control System ===\n\n");
    printf("Available Commands:\n");
    printf("  status                    - Show system status\n");
    printf("  ticket_list               - List all tickets\n");
    printf("  ticket_pay <id>           - Pay ticket\n");
    printf("  ticket_validate <id>      - Validate ticket for exit\n");
    printf("  gpio_read <gate> <dev>    - Read GPIO state\n");
    printf("  simulate_entry            - Simulate entry button press\n");
    printf("  simulate_exit             - Simulate car at exit\n");
    printf("  entry_barrier_block       - Block entry light barrier\n");
    printf("  entry_barrier_clear       - Clear entry light barrier\n");
    printf("  exit_barrier_block        - Block exit light barrier\n");
    printf("  exit_barrier_clear        - Clear exit light barrier\n");
    printf("  ?                         - Show this help\n");
    printf("  help                      - Show ESP-IDF help\n");
    printf("  clear                     - Clear screen\n");
    printf("  restart                   - Restart system\n");
    printf("\n");
    return 0;
}

void console_init(ParkingSystem* system) {
    g_system = system;

    // Register built-in help command first
    esp_console_register_help_command();

    // Register commands
    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Show system status",
        .hint = nullptr,
        .func = &cmd_status,
    };
    esp_console_cmd_register(&status_cmd);

    const esp_console_cmd_t ticket_list_cmd = {
        .command = "ticket_list",
        .help = "List all tickets",
        .hint = nullptr,
        .func = &cmd_ticket_list,
    };
    esp_console_cmd_register(&ticket_list_cmd);

    // ticket pay command
    ticket_pay_args.ticket_id = arg_int1(nullptr, nullptr, "<id>", "Ticket ID");
    ticket_pay_args.end = arg_end(2);

    const esp_console_cmd_t ticket_pay_cmd = {
        .command = "ticket_pay",
        .help = "Pay ticket",
        .hint = nullptr,
        .func = &cmd_ticket_pay,
        .argtable = &ticket_pay_args
    };
    esp_console_cmd_register(&ticket_pay_cmd);

    // ticket validate command
    ticket_validate_args.ticket_id = arg_int1(nullptr, nullptr, "<id>", "Ticket ID");
    ticket_validate_args.end = arg_end(2);

    const esp_console_cmd_t ticket_validate_cmd = {
        .command = "ticket_validate",
        .help = "Validate ticket",
        .hint = nullptr,
        .func = &cmd_ticket_validate,
        .argtable = &ticket_validate_args
    };
    esp_console_cmd_register(&ticket_validate_cmd);

    const esp_console_cmd_t gpio_read_cmd = {
        .command = "gpio_read",
        .help = "Read GPIO state",
        .hint = nullptr,
        .func = &cmd_gpio_read,
    };
    esp_console_cmd_register(&gpio_read_cmd);

    const esp_console_cmd_t simulate_entry_cmd = {
        .command = "simulate_entry",
        .help = "Simulate entry button press",
        .hint = nullptr,
        .func = &cmd_simulate_entry,
    };
    esp_console_cmd_register(&simulate_entry_cmd);

    const esp_console_cmd_t simulate_exit_cmd = {
        .command = "simulate_exit",
        .help = "Simulate car at exit",
        .hint = nullptr,
        .func = &cmd_simulate_exit,
    };
    esp_console_cmd_register(&simulate_exit_cmd);

    const esp_console_cmd_t help_cmd = {
        .command = "?",
        .help = "Show available commands",
        .hint = nullptr,
        .func = &cmd_help,
    };
    esp_console_cmd_register(&help_cmd);

    ESP_LOGI(TAG, "Console commands registered");
}

void console_start() {
    // Configure and start REPL
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "parking> ";
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
