#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Msz(std::vector<std::string>, Client &)
{
    std::pair<int, int> mapSize = _world->getMapSize();
    return "msz " + std::to_string(mapSize.first) + " " + std::to_string(mapSize.second) + "\n";
}
} // namespace zappy