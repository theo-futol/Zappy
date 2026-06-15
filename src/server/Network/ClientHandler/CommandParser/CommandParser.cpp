#include "CommandParser.hpp"
#include <sstream>
#include <sys/socket.h>

namespace zappy
{
CommandParser::CommandParser(Client *client, int f, World *world) : _client(client), _f(f), _world(world), _commands(world)
{
    _initAICommands();
    _initGraphicCommands();
}

void CommandParser::_initAICommands()
{
    _aiCommands["Forward"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Forward(cmd[1], client); });
    _aiCommands["Right"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Right(cmd[1], client); });
    _aiCommands["Left"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Left(cmd[1], client); });
    _aiCommands["Look"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Look(cmd[1], client); });
    _aiCommands["Inventory"] = std::make_pair(1, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Inventory(cmd[1], client); });
    _aiCommands["Connect_nbr"] = std::make_pair(0, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Connect_nbr(cmd[1], client); });
    _aiCommands["Broadcast"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Broadcast(cmd[1], client); });
    _aiCommands["Eject"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Eject(cmd[1], client); });
    _aiCommands["Take"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Take(cmd[1], client); });
    _aiCommands["Set"] = std::make_pair(7, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Set(cmd[1], client); });
    _aiCommands["Fork"] = std::make_pair(42, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Fork(cmd[1], client); });
    _aiCommands["Incantation"] = std::make_pair(300, [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Incantation(cmd[1], client); });
}

void CommandParser::_initGraphicCommands()
{
    _graphicCommands["ppo"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Ppo(cmd[1], client); };
    _graphicCommands["plv"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Plv(cmd[1], client); };
    _graphicCommands["pin"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Pin(cmd[1], client); };
    _graphicCommands["sgt"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Sgt(cmd[1], client); };
    _graphicCommands["sst"] = [](const std::vector<std::string> &cmd, Client &client, Commands &commands) { commands.Sst(cmd[1], client); };
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
        _commandQueue.push({line, base + std::chrono::milliseconds(cost * 1000 / _f)});
    }
    _client->setBuffer(buffer);
    return true;
}

void CommandParser::executeNext()
{
    if (_commandQueue.empty())
        return;
    if (std::chrono::steady_clock::now() < _commandQueue.front().readyAt)
        return;
    std::string line = _commandQueue.front().line;
    _commandQueue.pop();
    if (_client->getType() == ClientType::UNKNOWN)
        _handleHandshake(line);
    else
        _dispatch(line);
}

bool CommandParser::hasPending() const
{
    return !_commandQueue.empty();
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
        // here, need to tell the client playerID, map size
        send(_client->getFd(), "0\n10 10\n", 8, 0);
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
            it->second.second(args);
        else
            send(_client->getFd(), "ko\n", 3, 0);
    }
    else
    {
        auto it = _graphicCommands.find(cmd);
        if (it != _graphicCommands.end())
            it->second(args);
        else
            send(_client->getFd(), "suc\n", 4, 0);
    }
}
} // namespace zappy
