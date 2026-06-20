#include "CommandParser.hpp"
#include <sstream>
#include <sys/socket.h>

namespace zappy
{
CommandParser::CommandParser(Client *client, int f, World *world, std::queue<std::string> *broadcastQueue)
    : _client(client), _f(f), _world(world), _commands(world, broadcastQueue), _broadcastQueue(broadcastQueue), _isBanned(false)
{
    _initAICommands();
    _initGraphicCommands();
}

void CommandParser::_initAICommands()
{
    _aiCommands["Forward"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Forward(cmd, client); });
    _aiCommands["Right"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Right(cmd, client); });
    _aiCommands["Left"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Left(cmd, client); });
    _aiCommands["Look"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Look(cmd, client); });
    _aiCommands["Inventory"] = std::make_pair(1, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.getInventory(cmd, client); });
    _aiCommands["Connect_nbr"] = std::make_pair(0, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Connect_nbr(cmd, client); });
    _aiCommands["Broadcast"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Broadcast(cmd, client); });
    _aiCommands["Eject"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Eject(cmd, client); });
    _aiCommands["Take"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Take(cmd, client); });
    _aiCommands["Set"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Set(cmd, client); });
    _aiCommands["Fork"] = std::make_pair(42, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Fork(cmd, client); });
    _aiCommands["Incantation"] = std::make_pair(300, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Incantation(cmd, client); });
}

void CommandParser::_initGraphicCommands()
{
    _graphicCommands["msz"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Msz(cmd, client); };
    _graphicCommands["bct"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Bct(cmd, client); };
    _graphicCommands["mct"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Mct(cmd, client); };
    _graphicCommands["tna"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Tna(cmd, client); };
    _graphicCommands["ppo"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Ppo(cmd, client); };
    _graphicCommands["plv"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Plv(cmd, client); };
    _graphicCommands["pin"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Pin(cmd, client); };
    _graphicCommands["sgt"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Sgt(cmd, client); };
    _graphicCommands["sst"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { return commands.Sst(cmd, client); };
}

bool CommandParser::feed()
{
    char buf[4096] = {0};
    int bytes = recv(_client->getFd(), buf, sizeof(buf) - 1, 0);
    if (bytes <= 0)
        return false;
    std::string buffer = _client->getBuffer() + std::string(buf, bytes);
    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        if (line.empty())
            continue;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        auto it = _aiCommands.find(cmd);
        int cost = (it != _aiCommands.end()) ? it->second.first : 0;
        auto base = _commandQueue.empty() ? std::chrono::steady_clock::now() : _commandQueue.back().readyAt;
        auto readyAt = base + std::chrono::milliseconds(cost * 1000 / _f);
        Player *player = _world ? _world->getPlayerByFd(_client->getFd()) : nullptr;
        if (cmd == "Incantation" && _client->getType() == ClientType::AI && player && !player->isFrozen())
        {
            if (!_commands.beginIncantation(*_client, readyAt))
                continue;
        }
        _commandQueue.push({line, readyAt});
    }
    _client->setBuffer(buffer);
    return true;
}

bool CommandParser::executeNext()
{
    if (_commandQueue.empty())
        return false;
    if (_commandQueue.size() > 10) //< Ban system
    {
        _isBanned = true;
        return false;
    }
    if (_client->getType() == ClientType::DEAD)
    {
        _commandQueue.pop();
        return true;
    }
    if (_client->getType() == ClientType::AI)
    {
        Player *player = _world->getPlayerByFd(_client->getFd());
        if (player && player->getState() == PlayerState::DEAD)
        {
            _client->setType(ClientType::DEAD);
            _commandQueue.pop();
            return true;
        }
        // A frozen player (mid-incantation) must not run any queued command until the ritual ends.
        if (player && player->isFrozen())
            return false;
    }
    if (std::chrono::steady_clock::now() < _commandQueue.front().readyAt)
        return false;
    std::string line = _commandQueue.front().line;
    _commandQueue.pop();
    if (_client->getType() == ClientType::UNKNOWN)
        _handleHandshake(line);
    else
        _dispatch(line);
    return true;
}

std::chrono::steady_clock::time_point CommandParser::nextReadyAt() const
{
    if (_commandQueue.empty())
        return std::chrono::steady_clock::time_point::max();
    return _commandQueue.front().readyAt;
}

void CommandParser::_handleHandshake(const std::string &teamName)
{
    if (teamName == "GRAPHIC")
    {
        _client->setType(ClientType::GRAPHIC);
    }
    else
    {
        _client->setType(ClientType::AI);
        int availableSlots = _world->getAvailableSlotsForTeam(teamName);
        if (availableSlots < 0)
        {
            send(_client->getFd(), "ko\n", 3, 0);
            return;
        }
        std::string handShakeMsg = std::to_string(availableSlots) + "\n" + std::to_string(_world->getMapSize().first) + " " + std::to_string(_world->getMapSize().second) + "\n";
        send(_client->getFd(), handShakeMsg.c_str(), handShakeMsg.size(), 0);
        _world->addPlayer(_client->getFd(), teamName);

        _broadcastQueue->push("pnw " + std::to_string(_client->getFd()) + " " + std::to_string(_world->getPlayerByFd(_client->getFd())->getPosition().x) + " " +
                              std::to_string(_world->getPlayerByFd(_client->getFd())->getPosition().y) + " " +
                              std::to_string(_world->getPlayerByFd(_client->getFd())->getRotation()) + " " + teamName + "\n");
    }
}

void CommandParser::_dispatch(const std::string &line)
{
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg)
        args.push_back(arg);

    if (_client->getType() == ClientType::AI)
    {
        auto it = _aiCommands.find(cmd);
        if (it != _aiCommands.end())
        {
            std::string response = it->second.second(args, *_client, _commands);
            send(_client->getFd(), response.c_str(), response.size(), 0);
        }
        else
            send(_client->getFd(), "ko\n", 3, 0);
    }
    else
    {
        auto it = _graphicCommands.find(cmd);
        if (it != _graphicCommands.end())
        {
            std::string response = it->second(args, *_client, _commands);
            send(_client->getFd(), response.c_str(), response.size(), 0);
        }
        else
            send(_client->getFd(), "suc\n", 4, 0);
    }
}

bool CommandParser::isBanned() const
{
    return _isBanned;
}
} // namespace zappy
