#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Msz(std::vector<std::string> args, Client &client)
{
    (void)args;   // Unused parameter
    (void)client; // Unused parameter
    std::pair<int, int> mapSize = _world->getMapSize();
    return std::to_string(mapSize.first) + " " + std::to_string(mapSize.second) + "\n";
}
} // namespace zappy