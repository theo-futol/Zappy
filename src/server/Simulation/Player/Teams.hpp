#pragma once
#include "../Utils.hpp"
#include <memory>
#include <string>
#include <vector>

namespace zappy
{
static int eggID = 0;
struct Team
{
    std::string _name;
    int _teamID;
    int _slotsAvailable;                                      // Eggs will be counted as slots available for the team
    int _slotsOccupied;                                       // Number of players currently in the team
    std::vector<std::pair<position, std::vector<int>>> _eggs; // Vector of pair (position, list of egg IDs) representing the eggs of the team

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
            if (egg.first == position)
            {
                egg.second.emplace_back(eggID++);
                return;
            }
        std::vector<int> newEggs;
        for (int i = 0; i < count; ++i)
            newEggs.emplace_back(eggID++);
        _eggs.emplace_back(position, newEggs);
    }
    void removeEgg(const position &position, int count = 1, int eggID = -1)
    {
        _slotsAvailable -= count;
        for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
        {
            if (it->first == position)
            {
                if (count == -1)
                    it->second.clear();
                else
                {
                    if (eggID == -1)
                        it->second.resize(it->second.size() - count);
                    else
                        for (std::vector<int>::iterator eggIt = it->second.begin(); eggIt != it->second.end(); ++eggIt)
                            if (*eggIt == eggID)
                            {
                                it->second.erase(eggIt);
                                break;
                            }
                }
                return;
            }
        }
    }
    const std::vector<std::pair<position, std::vector<int>>> &getEggs() const
    {
        return _eggs;
    }
    bool hasEggAtPosition(const position &pos) const
    {
        for (const std::pair<position, std::vector<int>> &egg : _eggs)
            if (egg.first == pos && !egg.second.empty())
                return true;
        return false;
    }
};
} // namespace zappy
