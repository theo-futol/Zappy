#pragma once

#include <string>
#include <vector>

#include "Model/trantorian/Trantorian.hpp"
#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class TrantorianEventHandler
 * @brief Handles Trantorian-specific messages: pnw, plv, pin, pex, pfk, pdr, pgt, pipi.
 */
class TrantorianEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"pnw", "plv", "pin", "pex", "pfk", "pdr", "pgt", "pipi"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies a Trantorian message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;

  private:
    /**
     * @brief Creates a Trantorian from a pnw message.
     * @param args Message arguments: #n X Y O L N.
     * @param state State the player is added to.
     */
    void handleTrantorianSpawned(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Updates a player's level from a plv message.
     * @param args Message arguments: #n L.
     * @param state State holding the player.
     */
    void handleTrantorianLevelUpdated(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Applies the server's "pipi" full snapshot (position, orientation, level, inventory).
     * @param args Message arguments: #n X Y O L q0..q6.
     * @param state State holding the player (updated only, never created).
     */
    void handleTrantorianSnapshot(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Replaces a player's carried inventory from a pin message.
     * @param args Message arguments: #n X Y q0..q6.
     * @param state State holding the player.
     */
    void handleTrantorianInventoryUpdated(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Handles a pex message (expulsion); positions follow via ppo, so this is a no-op.
     * @param args Message arguments: #n.
     * @param state Unused state.
     */
    void handleTrantorianExpelled(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Handles a pfk message (fork); the egg arrives via enw, so this is a no-op.
     * @param args Message arguments: #n.
     * @param state Unused state.
     */
    void handleTrantorianForked(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Handles a pdr message: the player drops one resource onto its tile.
     * @param args Message arguments: #n i.
     * @param state State holding the player and the map.
     */
    void handleTrantorianResourceDropped(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Handles a pgt message: the player picks one resource up from its tile.
     * @param args Message arguments: #n i.
     * @param state State holding the player and the map.
     */
    void handleTrantorianResourceTaken(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Looks up a player and narrows it to its concrete Trantorian type.
     * @param state State holding the entity.
     * @param number Protocol player number.
     * @return The Trantorian, or nullptr if absent or of another type.
     */
    static Trantorian *getTrantorian(GameState &state, int number);

    /**
     * @brief Moves one resource unit between a player's inventory and its ground tile.
     * @param state State holding the player and the map.
     * @param args Message arguments: #n i.
     * @param toPlayer +1 to pick up (ground to inventory), -1 to drop (inventory to ground).
     */
    static void transferResource(const std::vector<std::string> &args, GameState &state, int toPlayer);
};

} // namespace Zappy
