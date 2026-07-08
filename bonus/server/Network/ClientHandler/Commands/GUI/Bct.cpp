#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Bct(std::vector<std::string> args, Client &)
{
    if (args.size() < 2)
        return "ko\n";
    int x, y;
    try
    {
        x = std::stoi(args[0]);
        y = std::stoi(args[1]);
    }
    catch (...)
    {
        return "ko\n";
    }
    tile *tilePtr = _world->getTileAt({x, y});
    if (!tilePtr)
        return "ko\n";
    std::string response = "bct " + std::to_string(x) + " " + std::to_string(y);
    // Exactly the 7 resources in protocol order — eggs sit on tiles too but are not part of bct.
    for (int type = static_cast<int>(ItemType::FOOD); type <= static_cast<int>(ItemType::THYSTAME); ++type)
    {
        int count = 0;
        for (const auto &item : tilePtr->_items)
            if (item.first == static_cast<ItemType>(type))
            {
                count = item.second;
                break;
            }
        response += " " + std::to_string(count);
    }
    response += "\n";
    return response;
}
} // namespace zappy