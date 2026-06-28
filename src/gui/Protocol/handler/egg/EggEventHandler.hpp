#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class EggEventHandler
 * @brief Handles egg-specific messages: laying (enw) and connection/hatch (ebo).
 */
class EggEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"enw", "ebo"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies an egg message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;

  private:
    /**
     * @brief Creates an egg from an enw message (egg laid by a player).
     * @param args Message arguments: #e #n X Y.
     * @param state State the egg is added to.
     */
    void handleEggLaid(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Removes a hatched egg from an ebo message (a client connected to it).
     * @param args Message arguments: #e.
     * @param state State the egg is removed from.
     */
    void handleEggHatch(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Resolves the team of an egg's parent player.
     * @param state State holding the parent entity.
     * @param parentNumber Protocol number of the parent player.
     * @return The parent's team name, or an empty string if it is unknown.
     */
    static std::string parentTeam(GameState &state, int parentNumber);
};

} // namespace Zappy
