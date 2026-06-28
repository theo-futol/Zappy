#include "Model/tile/Tile.hpp"

namespace Zappy
{

Tile::Tile() : _position{0, 0}, _resources()
{
}

GridPosition Tile::position() const
{
    return _position;
}

void Tile::setPosition(GridPosition position)
{
    _position = position;
}

ResourceSet &Tile::resources()
{
    return _resources;
}

const ResourceSet &Tile::resources() const
{
    return _resources;
}

bool Tile::incanting() const
{
    return _incanting;
}

void Tile::setIncanting(bool incanting)
{
    _incanting = incanting;
}

} // namespace Zappy
