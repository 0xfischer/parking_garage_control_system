#include "ConsoleHarness.h"
#include "ParkingGarageSystem.h"
#include "console_commands.h"
#include <vector>
#include <sstream>

// Mirror internal console state
static ParkingGarageSystem* s_system = nullptr;

static std::vector<std::string> split_args(const std::string& cmdline) {
    std::istringstream iss(cmdline);
    std::vector<std::string> args;
    std::string token;
    while (iss >> token)
        args.push_back(token);
    return args;
}

void console_test_init(ParkingGarageSystem* system) {
    s_system = system;
    console_init(system); // register commands (status, ticket, publish, gpio, ?)
}

int run_console_command(const std::string& cmdline) {
    if (!s_system)
        return -1;
    auto args_vec = split_args(cmdline);
    if (args_vec.empty())
        return -1;

    // Build argc/argv
    std::vector<char*> argv;
    argv.reserve(args_vec.size());
    for (auto& a : args_vec)
        argv.push_back(const_cast<char*>(a.c_str()));
    int argc = static_cast<int>(argv.size());

    const std::string& cmd = args_vec[0];
    if (cmd == "status")
        return cmd_status(argc, argv.data());
    if (cmd == "ticket")
        return cmd_ticket(argc, argv.data());
    if (cmd == "publish")
        return cmd_publish(argc, argv.data());
    if (cmd == "gpio")
        return cmd_gpio(argc, argv.data());
    if (cmd == "test")
        return cmd_test(argc, argv.data());
    if (cmd == "?" || cmd == "help")
        return cmd_help(argc, argv.data());
    return -2; // unknown command
}
