#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Bct(std::vector<std::string> args, Client &client)
{
    (void)client; // Unused parameter
    if (args.size() < 3)
        return "ko\n";
    int x = std::stoi(args[1]);
    int y = std::stoi(args[2]);
    tile *tilePtr = _world->getTileAt({x, y});
    if (!tilePtr)
        return "ko\n";
    std::string response = "bct " + std::to_string(x) + " " + std::to_string(y);
    for (const auto &item : tilePtr->_items)
        response += " " + std::to_string(item.second);
    response += "\n";
    return response;
}
} // namespace zappy