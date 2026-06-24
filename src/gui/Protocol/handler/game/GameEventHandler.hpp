#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class GameEventHandler
 * @brief Handles global/server messages: sgt, sst, seg, smg, suc, sbp and broadcast (pbc).
 */
class GameEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"sgt", "sst", "seg", "smg", "suc", "sbp", "pbc"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies a global/server message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;

  private:
    /**
     * @brief Updates the time unit from an sgt or sst message.
     * @param args Message arguments: T.
     * @param state State whose time unit is set.
     */
    void handleTimeUnit(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Records the winning team from a seg message (end of game).
     * @param args Message arguments: N.
     * @param state State whose winner is set.
     */
    void handleEndGame(const std::vector<std::string> &args, GameState &state);
};

} // namespace Zappy
