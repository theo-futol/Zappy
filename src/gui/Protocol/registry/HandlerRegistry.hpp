#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "interface/ICommandHandler.hpp"

namespace Zappy
{

/**
 * @class HandlerRegistry
 * @brief Owns the command handlers and routes messages to them by key.
 */
class HandlerRegistry
{
  public:
    HandlerRegistry();

    /**
     * @brief Registers a handler under each of its declared keys.
     * @param handler Handler to take ownership of.
     */
    void registerHandler(std::unique_ptr<ICommandHandler> handler);

    /**
     * @brief Routes a parsed message to its handler, ignoring unknown keys.
     * @param key The command key.
     * @param args The message arguments.
     * @param state The game state to mutate.
     */
    void dispatch(const std::string &key, const std::vector<std::string> &args, GameState &state);

  private:
    std::vector<std::unique_ptr<ICommandHandler>> _handlers; ///< Owned handlers.
    std::map<std::string, ICommandHandler *> _byKey;         ///< Dispatch table keyed by command.
};

} // namespace Zappy
