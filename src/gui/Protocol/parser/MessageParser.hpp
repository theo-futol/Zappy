#pragma once

#include <string>
#include <vector>

#include "Model/gamestate/GameState.hpp"
#include "Protocol/registry/HandlerRegistry.hpp"
#include "interface/IParser.hpp"

namespace Zappy
{

/**
 * @class MessageParser
 * @brief Splits the raw server stream into messages and dispatches them.
 *
 * Tolerant by design: an unknown or malformed line is ignored rather than
 * throwing, so a stray byte never crashes the GUI.
 */
class MessageParser : public IParser
{
  public:
    /**
     * @brief Builds the parser.
     * @param registry Dispatch target for parsed messages.
     * @param state Game state the handlers mutate.
     */
    MessageParser(HandlerRegistry &registry, GameState &state);

    /**
     * @brief Accumulates data and dispatches every complete line it contains.
     * @param data Newly received bytes.
     */
    void feed(const std::string &data) override;
  private:
  
    /**
   * @brief Splits a complete line into its command key and arguments.
   * @param line Line without its trailing newline.
   * @param key Output: the first token (the protocol command).
   * @param args Output: the remaining tokens.
   */
    void tokeniseLine(const std::string &line, std::string &key, std::vector<std::string> &args);
    HandlerRegistry &_registry; ///< Dispatch target.
    GameState &_state;          ///< State mutated by handlers.
    std::string _buffer;        ///< Accumulated, not-yet-complete bytes.
};

} // namespace Zappy
