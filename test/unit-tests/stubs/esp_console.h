#pragma once

// Minimal stub for host testing

struct esp_console_cmd_t {
    const char* command;
    const char* help;
    const char* hint;
    int (*func)(int argc, char** argv);
    void* argtable;
};

struct esp_console_repl_t {};
struct esp_console_repl_config_t {
    const char* prompt;
    int max_cmdline_length;
};
struct esp_console_dev_uart_config_t {};

#define ESP_CONSOLE_REPL_CONFIG_DEFAULT() \
    {                                     \
    }
#define ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT() \
    {                                         \
    }

inline void esp_console_register_help_command() {
}
inline int esp_console_cmd_register(const esp_console_cmd_t*) {
    return 0;
}
inline int esp_console_new_repl_uart(esp_console_dev_uart_config_t*, esp_console_repl_config_t*, esp_console_repl_t**) {
    return 0;
}
inline int esp_console_start_repl(esp_console_repl_t*) {
    return 0;
}
