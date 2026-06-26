#include "CommandParser.hpp"
#include "../../../Logger/Logger.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>

namespace zappy
{
CommandParser::CommandParser(Client *client, World *world, std::queue<std::string> *broadcastQueue)
    : _client(client), _world(world), _commands(world, broadcastQueue), _broadcastQueue(broadcastQueue), _isBanned(false)
{
    _initAICommands();
    _initGraphicCommands();
}

void CommandParser::_initAICommands()
{
    _aiCommands["Forward"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Forward(cmd, client, clients);
    });
    _aiCommands["Right"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Right(cmd, client, clients);
    });
    _aiCommands["Left"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Left(cmd, client, clients);
    });
    _aiCommands["Look"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Look(cmd, client, clients);
    });
    _aiCommands["Inventory"] = std::make_pair(1, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.getInventory(cmd, client, clients);
    });
    _aiCommands["Connect_nbr"] = std::make_pair(0, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Connect_nbr(cmd, client, clients);
    });
    _aiCommands["Broadcast"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Broadcast(cmd, client, clients);
    });
    _aiCommands["Eject"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Eject(cmd, client, clients);
    });
    _aiCommands["Take"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Take(cmd, client, clients);
    });
    _aiCommands["Set"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Set(cmd, client, clients);
    });
    _aiCommands["Fork"] = std::make_pair(42, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Fork(cmd, client, clients);
    });
    _aiCommands["Incantation"] = std::make_pair(300, [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients) {
        return commands.Incantation(cmd, client, clients);
    });
}

void CommandParser::_initGraphicCommands()
{
    _graphicCommands["msz"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Msz(cmd, client);
    };
    _graphicCommands["bct"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Bct(cmd, client);
    };
    _graphicCommands["mct"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Mct(cmd, client);
    };
    _graphicCommands["tna"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Tna(cmd, client);
    };
    _graphicCommands["ppo"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Ppo(cmd, client);
    };
    _graphicCommands["plv"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Plv(cmd, client);
    };
    _graphicCommands["pin"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Pin(cmd, client);
    };
    _graphicCommands["sgt"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Sgt(cmd, client);
    };
    _graphicCommands["sst"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &) {
        return commands.Sst(cmd, client);
    };
}

bool CommandParser::feed(std::vector<std::unique_ptr<Client>> &clients)
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
        Player *player = _world ? _world->getPlayerById(_client->getPlayerId()) : nullptr;
        Logger::log("110", "Parsing : input received", {{"player_id", player ? std::to_string(player->getId()) : std::to_string(_client->getFd())}});
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        auto it = _aiCommands.find(cmd);
        int cost = (it != _aiCommands.end()) ? it->second.first : 0;
        auto base = _commandQueue.empty() ? std::chrono::steady_clock::now() : _commandQueue.back().readyAt;
        auto readyAt = base + std::chrono::milliseconds(cost * 1000 / _world->getTimeUnit());
        if (cmd == "Incantation" && _client->getType() == ClientType::AI)
        {
            if (!player)
                continue;
            if (player->isFrozen())
            {
                Logger::log("8428", "Incantation : failed, player frozen during ritual", {{"player_id", std::to_string(player->getId())}});
                continue;
            }
            if (!_commands.beginIncantation(*_client, readyAt, clients))
                continue;
        }
        _commandQueue.push({line, readyAt});
    }
    _client->setBuffer(buffer);
    return true;
}

bool CommandParser::executeNext(std::vector<std::unique_ptr<Client>> &clients)
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
        std::istringstream commandName(_commandQueue.front().line);
        std::string command;
        commandName >> command;
        Logger::log("8442", "Player : action rejected, player is dead", {{"player_id", std::to_string(_client->getPlayerId())}, {"command", command}});
        send(_client->getFd(), "dead\n", 5, MSG_NOSIGNAL);
        _commandQueue.pop();
        return true;
    }
    if (_client->getType() == ClientType::AI)
    {
        Player *player = _world->getPlayerById(_client->getPlayerId());
        // A frozen player (mid-incantation) must not run any queued command until the ritual ends.
        if (player && player->isFrozen())
        {
            std::istringstream commandName(_commandQueue.front().line);
            std::string command;
            commandName >> command;
            // Logger::log("8441", "Player : action rejected, player is frozen", {{"player_id", std::to_string(player->getId())}, {"command", command}});
            return false;
        }
    }
    if (std::chrono::steady_clock::now() < _commandQueue.front().readyAt)
        return false;
    std::string line = _commandQueue.front().line;
    _commandQueue.pop();
    puts("ok");
    if (_client->getType() == ClientType::UNKNOWN)
        _handleHandshake(line);
    else
        _dispatch(line, clients);
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
        if (availableSlots <= 0)
        {
            std::cout << "Client refused: fd = " << _client->getFd() << " team=\"" << teamName << "\" reason=\"no available slot\"" << std::endl;
            send(_client->getFd(), "ko\n", 3, MSG_NOSIGNAL);
            return;
        }
        int playerId = _world->addPlayer(_client->getFd(), teamName);
        if (playerId < 0)
        {
            std::cout << "Client refused: fd = " << _client->getFd() << " team=\"" << teamName << "\" reason=\"no egg to hatch from\"" << std::endl;
            send(_client->getFd(), "ko\n", 3, MSG_NOSIGNAL);
            _isBanned = true;
            return;
        }
        _client->setPlayerId(playerId);
        std::string handShakeMsg = std::to_string(availableSlots) + "\n" + std::to_string(_world->getMapSize().first) + " " + std::to_string(_world->getMapSize().second) + "\n";
        send(_client->getFd(), handShakeMsg.c_str(), handShakeMsg.size(), MSG_NOSIGNAL);

        Player *player = _world->getPlayerById(playerId);
        _broadcastQueue->push("pnw " + std::to_string(playerId) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " " +
                              std::to_string(player->getOrientation()) + " " + std::to_string(player->getLevel()) + " " + teamName + "\n");
        _broadcastQueue->push(_commands.buildPipiMessage(*player));
    }
    // Log the handshake result
}

void CommandParser::_dispatch(const std::string &line, std::vector<std::unique_ptr<Client>> &clients)
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
            Logger::log("050", "Parsing : command valid", {{"player_id", std::to_string(_client->getPlayerId())}, {"command", cmd}});
            std::string response = it->second.second(args, *_client, _commands, clients);
            send(_client->getFd(), response.c_str(), response.size(), MSG_NOSIGNAL);
        }
        else
        {
            Logger::log("8410", "Parsing : unknown command", {{"player_id", std::to_string(_client->getPlayerId())}, {"input", line}});
            send(_client->getFd(), "ko\n", 3, MSG_NOSIGNAL);
        }
    }
    else
    {
        auto it = _graphicCommands.find(cmd);
        if (it != _graphicCommands.end())
        {
            Logger::log("050", "Parsing : command valid", {{"player_id", std::to_string(_client->getFd())}, {"command", cmd}});
            std::string response = it->second(args, *_client, _commands, clients);
            send(_client->getFd(), response.c_str(), response.size(), MSG_NOSIGNAL);
        }
        else
        {
            Logger::log("8410", "Parsing : unknown command", {{"player_id", std::to_string(_client->getFd())}, {"input", line}});
            send(_client->getFd(), "suc\n", 4, MSG_NOSIGNAL);
        }
    }
}

bool CommandParser::isBanned() const
{
    return _isBanned;
}
} // namespace zappy
