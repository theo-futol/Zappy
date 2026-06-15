#include "Commands.hpp"

namespace zappy
{
std::string Commands::Inventory(std::string command, Client &client)
{
    (void)command;                         // Unused parameter
    (void)client;                          // Unused parameter
    return "Inventory command executed\n"; // Placeholder return value
}
} // namespace zappy
