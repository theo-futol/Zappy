#pragma once

#include <string>

#include "types/Color.hpp"

namespace Zappy
{

/**
 * @struct LogMessage
 * @brief One line of the HUD message log: its text and display color.
 */
struct LogMessage
{
    std::string text; ///< Message text.
    Color color;      ///< Display color.
};

} // namespace Zappy
