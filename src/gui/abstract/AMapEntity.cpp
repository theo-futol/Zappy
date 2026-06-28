#include "abstract/AMapEntity.hpp"

#include <string>

namespace Zappy
{

AMapEntity::AMapEntity(int number, GridPosition position, Orientation orientation) : _number(number), _position(position), _orientation(orientation)
{
}

int AMapEntity::number() const
{
    return _number;
}

GridPosition AMapEntity::position() const
{
    return _position;
}

Orientation AMapEntity::orientation() const
{
    return _orientation;
}

int AMapEntity::level() const
{
    return 0;
}

void AMapEntity::setPosition(GridPosition position)
{
    _position = position;
}

void AMapEntity::setOrientation(Orientation orientation)
{
    _orientation = orientation;
}

std::vector<std::string> AMapEntity::infoLines() const
{
    return {getEntityType() + " #" + std::to_string(_number), "POS  " + std::to_string(_position.x) + ", " + std::to_string(_position.y)};
}

} // namespace Zappy
