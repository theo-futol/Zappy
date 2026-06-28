#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"
#include "types/Color.hpp"

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

    /**
     * @brief Logs a player broadcast (pbc) message.
     * @param args Message arguments: player number then the message words.
     * @param state State whose log is appended.
     */
    void handleBroadcast(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Logs a server message (smg).
     * @param args Message words.
     * @param state State whose log is appended.
     */
    void handleServerMessage(const std::vector<std::string> &args, GameState &state);

    static constexpr Color BroadcastColor{0.45f, 0.80f, 0.95f, 1.0f}; ///< Player broadcast color.
    static constexpr Color ServerColor{0.95f, 0.80f, 0.35f, 1.0f};    ///< Server message color.
};

} // namespace Zappy
