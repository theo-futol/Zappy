#include "Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client)
{
    (void)args;                              // Unused parameter
    (void)client;                            // Unused parameter
    return "Connect_nbr command executed\n"; // Placeholder return value
}
} // namespace zappy
