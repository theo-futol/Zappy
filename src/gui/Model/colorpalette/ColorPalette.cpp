#include "Model/colorpalette/ColorPalette.hpp"

namespace Zappy
{

ColorPalette::ColorPalette()
    : _colors{{1.0f, 0.2f, 0.2f, 1.0f}, {0.2f, 0.6f, 1.0f, 1.0f}, {0.2f, 0.8f, 0.3f, 1.0f}, {1.0f, 0.8f, 0.2f, 1.0f},
              {0.8f, 0.3f, 0.9f, 1.0f}, {0.2f, 0.9f, 0.9f, 1.0f}, {1.0f, 0.5f, 0.1f, 1.0f}, {0.9f, 0.9f, 0.9f, 1.0f}},
      _index(0)
{
}

Color ColorPalette::next()
{
    Color color = _colors[_index % _colors.size()];

    _index++;
    return color;
}

} // namespace Zappy
