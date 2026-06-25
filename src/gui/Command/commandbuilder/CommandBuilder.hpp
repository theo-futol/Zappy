#pragma once

#include <string>

namespace Zappy
{

/**
 * @class CommandBuilder
 * @brief Builds the outgoing protocol strings the GUI sends to the server.
 */
class CommandBuilder
{
  public:
    /** @brief Map size request. @return The "msz" command line. */
    static std::string requestMapSize();

    /** @brief Full map content request. @return The "mct" command line. */
    static std::string requestMapContent();

    /** @brief Team names request. @return The "tna" command line. */
    static std::string requestTeamNames();

    /**
     * @brief Single tile content request.
     * @param x Tile column.
     * @param y Tile row.
     * @return The "bct X Y" command line.
     */
    static std::string requestTileContent(int x, int y);

    /**
     * @brief Single player position request.
     * @param number Player number.
     * @return The "ppo #n" command line.
     */
    static std::string requestPlayerPosition(int number);

    /**
     * @brief Single player level request.
     * @param number Player number.
     * @return The "plv #n" command line.
     */
    static std::string requestPlayerLevel(int number);

    /**
     * @brief Single player inventory request.
     * @param number Player number.
     * @return The "pin #n" command line.
     */
    static std::string requestPlayerInventory(int number);

    /** @brief Time unit request. @return The "sgt" command line. */
    static std::string requestTimeUnit();

    /**
     * @brief Time unit modification.
     * @param timeUnit New time unit value.
     * @return The "sst T" command line.
     */
    static std::string setTimeUnit(int timeUnit);
};

} // namespace Zappy
