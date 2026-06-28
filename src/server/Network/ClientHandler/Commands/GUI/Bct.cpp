#include "../Commands.hpp"

namespace zappy
{
std::string Commands::buildBctMessage(position pos) const
{
    tile *tilePtr = _world->getTileAt(pos);
    std::string message = "bct " + std::to_string(pos.x) + " " + std::to_string(pos.y);

    if (!tilePtr)
        return message + "\n";
    for (const auto &item : tilePtr->_items)
        message += " " + std::to_string(item.second);
    message += "\n";
    return message;
}

std::string Commands::Bct(std::vector<std::string> args, Client &)
{
    if (args.size() < 2)
        return "sbp\n";
    int x = std::stoi(args[0]);
    int y = std::stoi(args[1]);
    if (!_world->getTileAt({x, y}))
        return "ko\n";
    return buildBctMessage({x, y});
}
} // namespace zappy
