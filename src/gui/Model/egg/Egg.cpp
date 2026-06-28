#include "Model/egg/Egg.hpp"

namespace Zappy
{

Egg::Egg(int number, GridPosition position, const std::string &team, Color color) : AMapEntity(number, position, Orientation::North), _team(team), _color(color)
{
}

std::string Egg::getEntityType() const
{
    return "egg";
}

Appearance Egg::appearance() const
{
    return Appearance{"egg", _color, 0.5f};
}

std::vector<std::string> Egg::infoLines() const
{
    std::vector<std::string> lines = AMapEntity::infoLines();

    lines.push_back("TEAM  " + _team);
    return lines;
}

const std::string &Egg::team() const
{
    return _team;
}

} // namespace Zappy
