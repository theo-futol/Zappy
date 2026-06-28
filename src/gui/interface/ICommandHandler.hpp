#pragma once

#include <string>
#include <vector>

#include "Model/gamestate/GameState.hpp"

namespace Zappy
{

/**
 * @class ICommandHandler
 * @brief Contract for translating one or more protocol messages into model mutations.
 *
 * A handler declares the keys it is responsible for; the registry routes matching
 * messages to it. Adding a message becomes "add a handler", with no change to the
 * parser or to existing handlers.
 */
class ICommandHandler
{
  public:
    virtual ~ICommandHandler() = default;

    /**
     * @brief Protocol keys this handler is responsible for.
     * @return The leading command tokens (e.g. {"pnw", "plv"}).
     */
    virtual std::vector<std::string> keys() const = 0;

    /**
     * @brief Applies a parsed message to the game state.
     * @param key The matched command key.
     * @param args The message arguments (key excluded).
     * @param state The game state to mutate.
     */
    virtual void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) = 0;
};

} // namespace Zappy
