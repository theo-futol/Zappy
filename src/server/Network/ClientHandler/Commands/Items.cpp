#include "Commands.hpp"

namespace zappy
{
std::string Commands::Take(std::string command, Client &client)
{
    (void)command;                    // Unused parameter
    (void)client;                     // Unused parameter
    return "Take command executed\n"; // Placeholder return value
}

std::string Commands::Set(std::string command, Client &client)
{
    (void)command;                   // Unused parameter
    (void)client;                    // Unused parameter
    return "Set command executed\n"; // Placeholder return value
}
} // namespace zappy
