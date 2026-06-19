#pragma once
#include <string>
#include <math.h>
#include <algorithm>
#include <chrono>

#include "../../ServerException/ServerException.hpp"
#include "Inventory/Inventory.hpp"
#include "Teams.hpp"
#include "../Utils.hpp"

#define BROADCAST_MESSAGE_TIME_PER_TILE 7

namespace zappy
{
/// @brief One AI-controlled creature in the world: its position, orientation, level,
///        team, inventory and life-cycle state, plus the bookkeeping for broadcast
///        messages it still has to deliver.
///
/// A Player is keyed to its client by the socket fd. Movement and direction helpers
/// take the map size so they can wrap around the toroidal world.
class Player
{
  private:
    int _fd; // Needed to link the player to its client

    int _level = 1;
    position _pos;
    int rotation;
    Team _team;
    bool _isLeveling;
    Inventory _inventory;
    PlayerState _state;
    std::vector<std::pair<std::string, std::vector<std::pair<std::pair<std::clock_t, int>, int>>>> _messagesToSend; // <<message, <<clock, timeNeeded>, receiverFd>, <clock, timeNeeded>, receiverFd>>, <message, <<clock, timeNeeded>, receiverFd>>>
    std::chrono::steady_clock::time_point _frozenUntil = std::chrono::steady_clock::time_point::min(); // While in the future, the player is frozen (e.g. during an incantation) and cannot act.

  public:
    /// @brief Spawns a player bound to a client fd, as a member of the given team.
    Player(int fd, const Team &team);

    /// @brief The team this player belongs to.
    const Team &getTeam() const;
    Team &getTeam();

    /// @brief Current tile position.
    const position &getPosition() const;
    /// @brief Current facing, in degrees (see Degrees).
    int getRotation() const;
    /// @brief Current elevation level (1..8).
    int getLevel() const;
    /// @brief This player's inventory.
    Inventory &getInventory();
    /// @brief Life-cycle state (pending / alive / dead).
    PlayerState getState() const;
    /// @brief The socket fd linking this player to its client.
    int getFd() const;

    /// @brief Tile the player would land on if it moved one step forward, with wrap-around.
    position nextPosition(std::pair<int, int> mapSize) const;
    /// @brief Sets the facing direction.
    void setRotation(int rotation);
    /// @brief Places the player at (x, y), wrapping into the map bounds.
    void setPosition(int x, int y, std::pair<int, int> mapSize);
    /// @brief Changes the life-cycle state.
    void changeState(PlayerState newState);
    /// @brief Moves one tile forward in the current facing, with wrap-around.
    void move(std::pair<int, int> mapSize);
    /// @brief Raises the player one elevation level.
    void levelUp();
    /// @brief Sends a raw message to this player's client socket.
    void writeToClient(const std::string &message) const;
    /// @brief Direction from this player toward a target tile, accounting for wrap-around.
    Degrees getDirectionTo(const position &target, std::pair<int, int> mapSize) const;
    /// @brief Sets the life-cycle state.
    void setState(PlayerState newState);
    /// @brief Freezes the player until the given time point (used while an incantation is underway).
    void setFrozenUntil(std::chrono::steady_clock::time_point until);
    /// @brief Returns true while the player is frozen and must not execute any command.
    bool isFrozen() const;
    /// @brief Direction from this player toward another player, accounting for wrap-around.
    Degrees getDirectionTo(const Player &target, std::pair<int, int> mapSize) const;
    /// @brief Shortest tile distance to a target position on the toroidal map.
    int getDistanceTo(const position &target, std::pair<int, int> mapSize) const;
    /// @brief Shortest tile distance to another player on the toroidal map.
    int getDistanceTo(const Player &target, std::pair<int, int> mapSize) const;
    // Broadcast a message to all players in the world, except the sender
    // TO DO : Use a class to represent the queue
    /// @brief Orders the pending-message queue by the time each message needs to arrive.
    void sortQueueByTimeNeeded();
    /// @brief Queues a message to be delivered to a receiver after timeNeeded elapses.
    void addMessageToQueue(const std::string &message, int timeNeeded, int receiverFd);
    /// @brief Flushes any queued messages whose delivery time has arrived.
    void sendMessageToClient();
  };
} // namespace zappy
