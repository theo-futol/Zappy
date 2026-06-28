#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Mct(std::vector<std::string>, Client &client)
{
    std::pair<int, int> mapSize = _world->getMapSize();
    std::string response;
    for (int x = 0; x < mapSize.first; ++x)
        for (int y = 0; y < mapSize.second; ++y)
            response += Bct({"bct", std::to_string(x), std::to_string(y)}, client);
    return response;
}
} // namespace zappy