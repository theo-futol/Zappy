#include "Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::vector<std::string> args, Client &client)
{
    (void)args;                            // Unused parameter
    (void)client;                          // Unused parameter
    return "Broadcast command executed\n"; // Placeholder return value
}
} // namespace zappy
