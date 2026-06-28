#pragma once

#include <cstddef>
#include <vector>

#include "Model/tile/Tile.hpp"

namespace Zappy
{

/**
 * @class Map
 * @brief Toroidal grid of tiles. Coordinates wrap on both axes.
 */
class Map
{
  public:
    Map();

    /**
     * @brief Resizes the map and (re)allocates its tiles.
     * @param width Grid width.
     * @param height Grid height.
     */
    void resize(int width, int height);

    /** @brief Map width. @return The number of columns. */
    int width() const;

    /** @brief Map height. @return The number of rows. */
    int height() const;

    /**
     * @brief Access to a tile, with toroidal wrapping of the coordinates.
     * @param x Column (wrapped modulo width).
     * @param y Row (wrapped modulo height).
     * @return The addressed tile.
     */
    Tile &at(int x, int y);

    /**
     * @brief Read-only access to a tile, with toroidal wrapping.
     * @param x Column (wrapped modulo width).
     * @param y Row (wrapped modulo height).
     * @return The addressed tile.
     */
    const Tile &at(int x, int y) const;

  private:
    /**
     * @brief Computes the row-major storage index for wrapped coordinates.
     * @param x Column.
     * @param y Row.
     * @return The index into the tile storage.
     */
    std::size_t index(int x, int y) const;

    int _width;               ///< Number of columns.
    int _height;              ///< Number of rows.
    std::vector<Tile> _tiles; ///< Row-major tile storage.
};

} // namespace Zappy
