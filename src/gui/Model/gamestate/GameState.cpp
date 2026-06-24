#include "Model/gamestate/GameState.hpp"

namespace Zappy
{

GameState::GameState() : _map(), _entities(), _teams(), _timeUnit(0), _winner(), _palette(), _teamColors()
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
}

const std::vector<std::string> &GameState::teams() const
{
    return _teams;
}

Color GameState::teamColor(const std::string &team)
{
    auto it = _teamColors.find(team);

    if (it != _teamColors.end())
        return it->second;

    Color color = _palette.next();

    _teamColors[team] = color;
    return color;
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

} // namespace Zappy
