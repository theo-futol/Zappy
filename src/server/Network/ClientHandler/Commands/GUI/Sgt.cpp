#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Sgt(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    std::string response = "sgt " + std::to_string(_world->getTimeUnit()) + "\n";
    send(client.getFd(), response.c_str(), response.size(), MSG_NOSIGNAL);
    return "";
}
} // namespace zappy
