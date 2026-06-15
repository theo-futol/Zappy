#include "Commands.hpp"

namespace zappy
{
std::string Commands::Take(std::vector<std::string> args, Client &client)
{
    (void)args;                       // Unused parameter
    (void)client;                     // Unused parameter
    return "Take command executed\n"; // Placeholder return value
}

std::string Commands::Set(std::vector<std::string> args, Client &client)
{
    (void)args;                      // Unused parameter
    (void)client;                    // Unused parameter
    return "Set command executed\n"; // Placeholder return value
}
} // namespace zappy
