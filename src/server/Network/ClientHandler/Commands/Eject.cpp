#include "Commands.hpp"

namespace zappy
{
std::string Commands::Eject(std::vector<std::string> args, Client &client)
{
    (void)args;                        // Unused parameter
    (void)client;                      // Unused parameter
    return "Eject command executed\n"; // Placeholder return value
}
} // namespace zappy
