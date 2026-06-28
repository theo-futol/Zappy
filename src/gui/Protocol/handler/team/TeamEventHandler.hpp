#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class TeamEventHandler
 * @brief Handles the team name message (tna).
 */
class TeamEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"tna"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies a team message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;
};

} // namespace Zappy
