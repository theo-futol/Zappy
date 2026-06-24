#pragma once

#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @struct NamedColors
 * @brief Human-readable RGB color constants, so code reads color names instead of raw floats.
 */
struct NamedColors
{
    static constexpr Vec3 White = Vec3(1.0f, 1.0f, 1.0f);     ///< Pure white.
    static constexpr Vec3 LightGray = Vec3(0.7f, 0.7f, 0.7f); ///< Light gray.
    static constexpr Vec3 DarkGray = Vec3(0.4f, 0.4f, 0.4f);  ///< Dark gray.
    static constexpr Vec3 OffWhite = Vec3(0.9f, 0.9f, 0.85f); ///< Slightly warm white.
    static constexpr Vec3 Green = Vec3(0.2f, 0.8f, 0.2f);     ///< Green.
    static constexpr Vec3 Cyan = Vec3(0.2f, 0.7f, 0.8f);      ///< Cyan.
    static constexpr Vec3 Brown = Vec3(0.55f, 0.35f, 0.15f);  ///< Brown.
    static constexpr Vec3 Purple = Vec3(0.6f, 0.2f, 0.8f);    ///< Purple.
    static constexpr Vec3 Orange = Vec3(0.9f, 0.5f, 0.1f);    ///< Orange.
    static constexpr Vec3 Red = Vec3(0.9f, 0.2f, 0.3f);       ///< Red.
};

} // namespace Zappy
