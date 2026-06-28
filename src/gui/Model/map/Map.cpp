#include "Model/map/Map.hpp"

namespace Zappy
{

Map::Map() : _width(0), _height(0), _tiles()
{
}

void Map::resize(int width, int height)
{
    _width = width;
    _height = height;
    _tiles.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), Tile());
}

int Map::width() const
{
    return _width;
}

int Map::height() const
{
    return _height;
}

std::size_t Map::index(int x, int y) const
{
    if (_width == 0 || _height == 0)
        return 0;
    int wrappedX = ((x % _width) + _width) % _width;
    int wrappedY = ((y % _height) + _height) % _height;

    return static_cast<std::size_t>(wrappedY) * static_cast<std::size_t>(_width) + static_cast<std::size_t>(wrappedX);
}

Tile &Map::at(int x, int y)
{
    return _tiles[index(x, y)];
}

const Tile &Map::at(int x, int y) const
{
    return _tiles[index(x, y)];
}

} // namespace Zappy
