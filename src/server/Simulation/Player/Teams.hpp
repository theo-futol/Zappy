#pragma once
#include "../Utils.hpp"
#include <memory>
#include <string>
#include <vector>

namespace zappy
{
/// @brief Process-wide counter handing out unique IDs to every egg laid.
static int eggID = 0;

/// @brief One team in the game: its name, its connection slots, and its eggs.
///
/// A free connection slot is the right to connect a new AI player. Slots come from
/// the initial count plus every egg laid (each egg adds a slot), so the egg list and
/// the slot counters are kept consistent by addEgg()/removeEgg().
struct Team
{
    std::string _name;
    int _teamID;
    int _slotsAvailable;                                      // Eggs will be counted as slots available for the team
    int _slotsOccupied;                                       // Number of players currently in the team
    std::vector<std::pair<position, std::vector<int>>> _eggs; // Vector of pair (position, list of egg IDs) representing the eggs of the team
    bool _hasWin;                                             // Indicate if the team has won the game

    Team(const std::string &name, int teamID, int initialSlots) : _name(name), _teamID(teamID), _slotsAvailable(initialSlots), _slotsOccupied(0), _eggs(), _hasWin(false)
    {
    }
    /// @brief True if a new player may still connect to this team.
    bool hasAvailableSlots() const
    {
        return _slotsOccupied < _slotsAvailable;
    }
    /// @brief Number of free connection slots remaining.
    int getAvailableSlots() const
    {
        return _slotsAvailable - _slotsOccupied;
    }
    /// @brief Marks one slot as taken (a player connected).
    void addPlayer()
    {
        ++_slotsOccupied;
    }
    /// @brief Frees one occupied slot (a player left), never going below zero.
    void removePlayer()
    {
        if (_slotsOccupied > 0){
            --_slotsOccupied;
            --_slotsAvailable;
        }
    }
    /// @brief Lays count eggs at the given tile, each adding a slot and a unique ID.
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
    /// @brief Removes eggs at a tile (and their slots). With eggID set, removes just
    ///        that one egg; with count == -1, clears every egg on the tile.
    void removeEgg(const position &position, int count = 1, int eggID = -1)
    {
        if (count == 0)
            return;
        int slotsToRemove = 0;
        for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
        {
            if (it->first == position)
            {
                if (count == -1)
                {
                    slotsToRemove = it->second.size();
                    it->second.clear();
                }
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
                break;
            }
        }
        _slotsAvailable -= slotsToRemove == 0 ? count : slotsToRemove;
    }
    /// @brief Read-only view of all eggs (tile position paired with its egg IDs).
    const std::vector<std::pair<position, std::vector<int>>> &getEggs() const
    {
        return _eggs;
    }
    /// @brief True if at least one egg is currently sitting on the given tile.
    bool hasEggAtPosition(const position &pos) const
    {
        for (const std::pair<position, std::vector<int>> &egg : _eggs)
            if (egg.first == pos && !egg.second.empty())
                return true;
        return false;
    }
};
} // namespace zappy
