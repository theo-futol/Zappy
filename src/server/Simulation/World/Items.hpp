#pragma once
#include <string>

namespace zappy
{
    enum class ItemType
    {
        FOOD,
        LINEMATE,
        DERAUMERE,
        SIBUR,
        MENDIANE,
        PHIRAS,
        THYSTAME
    };

    inline std::string itemTypeToString(ItemType type)
    {
        switch (type) {
            case ItemType::FOOD: return "food";
            case ItemType::LINEMATE: return "linemate";
            case ItemType::DERAUMERE: return "deraumere";
            case ItemType::SIBUR: return "sibur";
            case ItemType::MENDIANE: return "mendiane";
            case ItemType::PHIRAS: return "phiras";
            case ItemType::THYSTAME: return "thystame";
            default: return "unknown";
        }
    }
} // namespace zappy
