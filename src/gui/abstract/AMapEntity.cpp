#include "abstract/AMapEntity.hpp"

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

void AMapEntity::setPosition(GridPosition position)
{
    _position = position;
}

void AMapEntity::setOrientation(Orientation orientation)
{
    _orientation = orientation;
}

} // namespace Zappy
