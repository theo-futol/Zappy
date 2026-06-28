#include "Model/gamestate/GameState.hpp"

#include <algorithm>
#include <iterator>

namespace Zappy
{

GameState::GameState()
    : _map(), _entities(), _teams(), _timeUnit(0), _winner(), _palette(), _teamColors(), _messages(), _selectedKey(), _hasSelection(false), _hoveredTile(), _hasHover(false)
{
}

Map &GameState::map()
{
    return _map;
}

const Map &GameState::map() const
{
    return _map;
}

void GameState::addEntity(std::unique_ptr<IEntity> entity)
{
    EntityKey key{entity->getEntityType(), entity->number()};

    _entities[key] = std::move(entity);
}

void GameState::removeEntity(const EntityKey &key)
{
    _entities.erase(key);
}

IEntity *GameState::getEntity(const EntityKey &key)
{
    auto it = _entities.find(key);

    if (it == _entities.end())
        return nullptr;
    return it->second.get();
}

const std::map<EntityKey, std::unique_ptr<IEntity>> &GameState::entities() const
{
    return _entities;
}

void GameState::addTeam(const std::string &team)
{
    _teams.push_back(team);
    if (_teamColors.find(team) == _teamColors.end())
        _teamColors[team] = _palette.next();
}

const std::vector<std::string> &GameState::teams() const
{
    return _teams;
}

Color GameState::teamColor(const std::string &team) const
{
    auto it = _teamColors.find(team);

    if (it != _teamColors.end())
        return it->second;
    return Color{1.0f, 1.0f, 1.0f, 1.0f};
}

std::size_t GameState::teamIndex(const std::string &team) const
{
    auto it = std::find(_teams.begin(), _teams.end(), team);

    if (it != _teams.end())
        return static_cast<std::size_t>(std::distance(_teams.begin(), it));
    return 0;
}

void GameState::setTimeUnit(int timeUnit)
{
    _timeUnit = timeUnit;
}

int GameState::timeUnit() const
{
    return _timeUnit;
}

void GameState::setWinner(const std::string &team)
{
    _winner = team;
}

const std::string &GameState::winner() const
{
    return _winner;
}

void GameState::addMessage(const std::string &text, Color color)
{
    _messages.push_back(LogMessage{text, color});
    if (_messages.size() > MaxMessages)
        _messages.erase(_messages.begin());
}

const std::vector<LogMessage> &GameState::messages() const
{
    return _messages;
}

void GameState::selectEntity(const EntityKey &key)
{
    _selectedKey = key;
    _hasSelection = true;
}

void GameState::clearSelection()
{
    _hasSelection = false;
}

const IEntity *GameState::selectedEntity() const
{
    if (!_hasSelection)
        return nullptr;

    auto it = _entities.find(_selectedKey);

    if (it == _entities.end())
        return nullptr;
    return it->second.get();
}

void GameState::setHoveredTile(GridPosition tile)
{
    _hoveredTile = tile;
    _hasHover = true;
}

void GameState::clearHoveredTile()
{
    _hasHover = false;
}

bool GameState::hasHoveredTile() const
{
    return _hasHover;
}

GridPosition GameState::hoveredTile() const
{
    return _hoveredTile;
}

} // namespace Zappy
