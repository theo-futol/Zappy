#include "Commands.hpp"

namespace zappy
{
std::string Commands::Eject(std::string command, Client &client)
{
    (void)command;                     // Unused parameter
    (void)client;                      // Unused parameter
    return "Eject command executed\n"; // Placeholder return value
}
} // namespace zappy
