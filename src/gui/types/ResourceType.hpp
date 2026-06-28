#pragma once

namespace Zappy
{

/**
 * @enum ResourceType
 * @brief The seven resource categories of Trantor, matching protocol fields q0..q6.
 */
enum class ResourceType
{
    Food = 0,
    Linemate = 1,
    Deraumere = 2,
    Sibur = 3,
    Mendiane = 4,
    Phiras = 5,
    Thystame = 6
};

/**
 * @brief Human-readable name of a resource type.
 * @param type Resource type.
 * @return The display name (e.g. "Linemate").
 */
inline const char *resourceName(ResourceType type)
{
    switch (type)
    {
    case ResourceType::Food:
        return "Food";
    case ResourceType::Linemate:
        return "Linemate";
    case ResourceType::Deraumere:
        return "Deraumere";
    case ResourceType::Sibur:
        return "Sibur";
    case ResourceType::Mendiane:
        return "Mendiane";
    case ResourceType::Phiras:
        return "Phiras";
    case ResourceType::Thystame:
        return "Thystame";
    }
    return "?";
}

} // namespace Zappy
