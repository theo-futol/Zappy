#include "Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::string command, Client &client)
{
    (void)command;                         // Unused parameter
    (void)client;                          // Unused parameter
    return "Broadcast command executed\n"; // Placeholder return value
}
} // namespace zappy
