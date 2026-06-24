#pragma once

#include <string>
#include <vector>

#include "Model/gamestate/GameState.hpp"
#include "abstract/AEventHandler.hpp"

namespace Zappy
{

/**
 * @class MapEventHandler
 * @brief Handles map-related messages: size (msz), tile content (bct), full map (mct).
 */
class MapEventHandler : public AEventHandler
{
  public:
    /** @brief Keys handled. @return {"msz", "bct", "mct"}. */
    std::vector<std::string> keys() const override;

    /**
     * @brief Applies a map message to the state.
     * @param key Matched key.
     * @param args Message arguments.
     * @param state State to mutate.
     */
    void handle(const std::string &key, const std::vector<std::string> &args, GameState &state) override;
  private:
    /**
     * @brief Resizes the map from a msz message.
     * @param args Message arguments: X Y.
     * @param state State whose map is resized.
     */
    void handleMapSize(const std::vector<std::string> &args, GameState &state);

    /**
     * @brief Fills one tile's ground resources from a bct message.
     * @param args Message arguments: X Y q0..q6.
     * @param state State whose tile is updated.
     */
    void handleTileContent(const std::vector<std::string> &args, GameState &state);
};

} // namespace Zappy
