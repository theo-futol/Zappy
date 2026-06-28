#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class EntityCommonHandler
 * @brief Handles attributes common to any entity: position (ppo) and removal (pdi, edi).
 *
 * Operating through IEntity, it works for any present or future entity type
 * without modification.
 */
class EntityCommonHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"ppo", "pdi", "edi"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies a common entity message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;

  private:
    /**
     * @brief Updates a player's position and orientation from a ppo message.
     * @param args Message arguments: #n X Y O.
     * @param state State holding the entity to move.
     */
    void handlePosition(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Removes an entity from the state (pdi for players, edi for eggs).
     * @param args Message arguments: #n.
     * @param state State to remove the entity from.
     * @param entityType Entity category to target ("player" or "egg").
     */
    void handleRemoval(const std::vector<std::string> &args, GameState &state, const std::string &entityType);
};

} // namespace Zappy
