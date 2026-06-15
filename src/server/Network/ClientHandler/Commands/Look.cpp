#include "Commands.hpp"

namespace zappy
{
std::string Commands::Look(std::string command, Client &client)
{
    (void)command;                    // Unused parameter
    (void)client;                     // Unused parameter
    return "Look command executed\n"; // Placeholder return value
}
} // namespace zappy
