#pragma once

#include <string>
#include <vector>

#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class IncantationEventHandler
 * @brief Handles incantation messages: start (pic) and end (pie).
 */
class IncantationEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"pic", "pie"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies an incantation message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;
};

} // namespace Zappy
