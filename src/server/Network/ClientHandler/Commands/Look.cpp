#include "Commands.hpp"

namespace zappy
{
std::string Commands::Look(std::vector<std::string> args, Client &client)
{
    (void)args;                       // Unused parameter
    (void)client;                     // Unused parameter
    return "Look command executed\n"; // Placeholder return value
}
} // namespace zappy
