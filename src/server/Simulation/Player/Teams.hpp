#pragma once
#include <string>
#include <memory>
#include <vector>
#include "../Player/Player.hpp"

namespace zappy
{
    struct Team
    {
            std::string _name;
            int _teamID;
            int slotsAvailable;

            Team(const std::string &name, int teamID, int initialSlots) : _name(name), _teamID(teamID), slotsAvailable(initialSlots) {}
    };
} // namespace zappy
