#include "Command/commandbuilder/CommandBuilder.hpp"

namespace Zappy
{

std::string CommandBuilder::requestMapSize()
{
    return "msz\n";
}

std::string CommandBuilder::requestMapContent()
{
    return "mct\n";
}

std::string CommandBuilder::requestTeamNames()
{
    return "tna\n";
}

std::string CommandBuilder::requestTileContent(int x, int y)
{
    return "bct " + std::to_string(x) + " " + std::to_string(y) + "\n";
}

std::string CommandBuilder::requestPlayerPosition(int number)
{
    return "ppo #" + std::to_string(number) + "\n";
}

std::string CommandBuilder::requestPlayerLevel(int number)
{
    return "plv #" + std::to_string(number) + "\n";
}

std::string CommandBuilder::requestPlayerInventory(int number)
{
    return "pin #" + std::to_string(number) + "\n";
}

std::string CommandBuilder::requestTimeUnit()
{
    return "sgt\n";
}

std::string CommandBuilder::setTimeUnit(int timeUnit)
{
    return "sst " + std::to_string(timeUnit) + "\n";
}

} // namespace Zappy
