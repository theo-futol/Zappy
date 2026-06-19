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
        THYSTAME,
        EGG,
        UNKNOWN
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
            case ItemType::EGG: return "egg";
            default: return "unknown";
        }
    }

    inline ItemType stringToItemType(const std::string &str)
    {
        if (str == "food") return ItemType::FOOD;
        if (str == "linemate") return ItemType::LINEMATE;
        if (str == "deraumere") return ItemType::DERAUMERE;
        if (str == "sibur") return ItemType::SIBUR;
        if (str == "mendiane") return ItemType::MENDIANE;
        if (str == "phiras") return ItemType::PHIRAS;
        if (str == "thystame") return ItemType::THYSTAME;
        if (str == "egg") return ItemType::EGG;
        return ItemType::UNKNOWN;
    }
} // namespace zappy
