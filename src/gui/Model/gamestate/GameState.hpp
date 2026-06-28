#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Model/colorpalette/ColorPalette.hpp"
#include "Model/map/Map.hpp"
#include "interface/IEntity.hpp"
#include "types/Color.hpp"
#include "types/EntityKey.hpp"
#include "types/GridPosition.hpp"
#include "types/LogMessage.hpp"

namespace Zappy
{

/**
 * @class GameState
 * @brief Single source of truth for the GUI: map, entities, teams, time and winner.
 *
 * Mutated by the protocol handlers, read (const) by the renderers. Entities are
 * stored homogeneously as IEntity so adding a new entity type never changes this
 * class.
 */
class GameState
{
  public:
    GameState();

    /** @brief Mutable access to the map. @return The map. */
    Map &map();

    /** @brief Read-only access to the map. @return The map. */
    const Map &map() const;

    /**
     * @brief Inserts or replaces an entity, keyed by its entity type and number.
     * @param entity Entity to store (ownership transferred).
     */
    void addEntity(std::unique_ptr<IEntity> entity);

    /**
     * @brief Removes an entity by key.
     * @param key Identity of the entity to remove.
     */
    void removeEntity(const EntityKey &key);

    /**
     * @brief Looks up an entity by key.
     * @param key Identity of the entity.
     * @return Pointer to the entity, or nullptr if absent.
     */
    IEntity *getEntity(const EntityKey &key);

    /** @brief Read-only view of all entities. @return The entity map. */
    const std::map<EntityKey, std::unique_ptr<IEntity>> &entities() const;

    /**
     * @brief Registers a team name.
     * @param team Team to add.
     */
    void addTeam(const std::string &team);

    /** @brief Read-only view of the team names. @return The teams. */
    const std::vector<std::string> &teams() const;

    /**
     * @brief Distinct display color assigned to a team (when it was registered).
     * @param team Team name.
     * @return The team's color, or white if the team is unknown.
     */
    Color teamColor(const std::string &team) const;

    /**
     * @brief Arrival index of a team, i.e. its position in registration order.
     * @param team Team name.
     * @return The 0-based index, or 0 if the team is unknown.
     */
    std::size_t teamIndex(const std::string &team) const;

    /** @brief Sets the current time unit. @param timeUnit New time unit. */
    void setTimeUnit(int timeUnit);

    /** @brief Current time unit. @return The time unit. */
    int timeUnit() const;

    /** @brief Sets the winning team. @param team Winning team name. */
    void setWinner(const std::string &team);

    /** @brief Winning team, empty while the game is running. @return The winner. */
    const std::string &winner() const;

    /**
     * @brief Appends a message to the bounded log (oldest dropped past the cap).
     * @param text Message text.
     * @param color Display color.
     */
    void addMessage(const std::string &text, Color color);

    /** @brief Most recent log messages (broadcasts, server messages). @return The log. */
    const std::vector<LogMessage> &messages() const;

    /**
     * @brief Marks an entity as the selected one (its info is shown in the HUD).
     * @param key Identity of the entity to select.
     */
    void selectEntity(const EntityKey &key);

    /** @brief Clears the current selection. */
    void clearSelection();

    /**
     * @brief Currently selected entity, resolved live (null if none or it is gone).
     * @return Pointer to the selected entity, or nullptr.
     */
    const IEntity *selectedEntity() const;

    /**
     * @brief Sets the tile the cursor currently hovers (for highlighting).
     * @param tile Hovered tile coordinates.
     */
    void setHoveredTile(GridPosition tile);

    /** @brief Clears the hovered tile (cursor off the map). */
    void clearHoveredTile();

    /** @brief Whether a tile is currently hovered. @return True if hovering a tile. */
    bool hasHoveredTile() const;

    /** @brief Currently hovered tile (valid only when hasHoveredTile()). @return The tile. */
    GridPosition hoveredTile() const;

  private:
    static constexpr std::size_t MaxMessages = 6; ///< Cap on retained log lines.

    Map _map;                                                ///< Toroidal world map.
    std::map<EntityKey, std::unique_ptr<IEntity>> _entities; ///< All map entities.
    std::vector<std::string> _teams;                         ///< Known team names.
    int _timeUnit;                                           ///< Current server time unit.
    std::string _winner;                                     ///< Winning team, if any.
    ColorPalette _palette;                                   ///< Generator of distinct team colors.
    std::map<std::string, Color> _teamColors;                ///< Assigned color per team.
    std::vector<LogMessage> _messages;                       ///< Bounded log of recent messages.
    EntityKey _selectedKey;                                  ///< Identity of the selected entity.
    bool _hasSelection;                                      ///< Whether an entity is selected.
    GridPosition _hoveredTile;                               ///< Tile under the cursor (when hovering).
    bool _hasHover;                                          ///< Whether a tile is hovered.
};

} // namespace Zappy
