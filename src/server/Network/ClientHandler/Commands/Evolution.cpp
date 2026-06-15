#include "Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::string command, Client &client)
{
    (void)command;                    // Unused parameter
    (void)client;                     // Unused parameter
    return "Fork command executed\n"; // Placeholder return value
}

std::string Commands::Incantation(std::string command, Client &client)
{
    (void)command;                           // Unused parameter
    (void)client;                            // Unused parameter
    return "Incantation command executed\n"; // Placeholder return value
}
} // namespace zappy
