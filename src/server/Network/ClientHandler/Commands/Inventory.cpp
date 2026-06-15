#include "Commands.hpp"

namespace zappy
{
std::string Commands::getInventory(std::vector<std::string> args, Client &client)
{
    (void)args;                            // Unused parameter
    (void)client;                          // Unused parameter
    return "Inventory command executed\n"; // Placeholder return value
}
} // namespace zappy
