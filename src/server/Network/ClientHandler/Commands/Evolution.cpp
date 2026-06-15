#include "Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::vector<std::string> args, Client &client)
{
    (void)args;                       // Unused parameter
    (void)client;                     // Unused parameter
    return "Fork command executed\n"; // Placeholder return value
}

std::string Commands::Incantation(std::vector<std::string> args, Client &client)
{
    (void)args;                              // Unused parameter
    (void)client;                            // Unused parameter
    return "Incantation command executed\n"; // Placeholder return value
}
} // namespace zappy
