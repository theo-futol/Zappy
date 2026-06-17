#pragma once
#include "../Utils.hpp"
#include <memory>
#include <string>
#include <vector>

namespace zappy
{
struct Team
{
    std::string _name;
    int _teamID;
    int _slotsAvailable;                         // Eggs will be counted as slots available for the team
    int _slotsOccupied;                          // Number of players currently in the team
    std::vector<std::pair<position, int>> _eggs; // Vector of position with available eggs at the position

    Team(const std::string &name, int teamID, int initialSlots) : _name(name), _teamID(teamID), _slotsAvailable(initialSlots), _slotsOccupied(0), _eggs()
    {
    }
    bool hasAvailableSlots() const
    {
        return _slotsOccupied < _slotsAvailable;
    }
    int getAvailableSlots() const
    {
        return _slotsAvailable - _slotsOccupied;
    }
    void addPlayer()
    {
        ++_slotsOccupied;
    }
    void removePlayer()
    {
        if (_slotsOccupied > 0)
            --_slotsOccupied;
    }
    void addEgg(const position &position, int count = 1)
    {
        _slotsAvailable += count;
        for (auto &egg : _eggs)
        {
            if (egg.first == position)
            {
                egg.second += count;
                return;
            }
        }
        _eggs.emplace_back(position, count);
    }
    void removeEgg(const position &position, int count = 1)
    {
        _slotsAvailable -= count;
        for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
        {
            if (it->first == position)
            {
                if (count == -1)
                    it->second = 0;
                else
                    it->second -= count;
                return;
            }
        }
    }
    const std::vector<std::pair<position, int>> &getEggs() const
    {
        return _eggs;
    }
    bool hasEggAtPosition(const position &pos) const
    {
        for (const auto &egg : _eggs)
            if (egg.first == pos && egg.second > 0)
                return true;
        return false;
    }
};
} // namespace zappy
