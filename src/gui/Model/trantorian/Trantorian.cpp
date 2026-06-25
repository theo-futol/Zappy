#include "Model/trantorian/Trantorian.hpp"

namespace Zappy
{

Trantorian::Trantorian(int number, GridPosition position, Orientation orientation, const std::string &team, int level, Color color)
    : AMapEntity(number, position, orientation), _team(team), _level(level), _inventory(), _color(color)
{
}

std::string Trantorian::getEntityType() const
{
    return "player";
}

Appearance Trantorian::appearance() const
{
    return Appearance{"trantorian", _color, 0.8f + static_cast<float>(_level) * 0.06f};
}

std::vector<std::string> Trantorian::infoLines() const
{
    std::vector<std::string> lines = AMapEntity::infoLines();

    lines.push_back("TEAM  " + _team);
    lines.push_back("LEVEL  " + std::to_string(_level));
    lines.push_back("INV  F" + std::to_string(_inventory.get(ResourceType::Food)) + " L" + std::to_string(_inventory.get(ResourceType::Linemate)) + " D" +
                    std::to_string(_inventory.get(ResourceType::Deraumere)) + " S" + std::to_string(_inventory.get(ResourceType::Sibur)));
    lines.push_back("     M" + std::to_string(_inventory.get(ResourceType::Mendiane)) + " P" + std::to_string(_inventory.get(ResourceType::Phiras)) + " T" +
                    std::to_string(_inventory.get(ResourceType::Thystame)));
    return lines;
}

const std::string &Trantorian::team() const
{
    return _team;
}

int Trantorian::level() const
{
    return _level;
}

void Trantorian::setLevel(int level)
{
    _level = level;
}

ResourceSet &Trantorian::inventory()
{
    return _inventory;
}

const ResourceSet &Trantorian::inventory() const
{
    return _inventory;
}

} // namespace Zappy
