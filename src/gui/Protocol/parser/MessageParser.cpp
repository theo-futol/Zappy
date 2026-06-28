#include "Protocol/parser/MessageParser.hpp"
#include <sstream>

namespace Zappy
{

MessageParser::MessageParser(HandlerRegistry &registry, GameState &state) : _registry(registry), _state(state), _buffer()
{
}

void MessageParser::tokeniseLine(const std::string &line, std::string &key, std::vector<std::string> &args)
{
    std::istringstream iss(line);
    std::string token;

    args.clear();
    iss >> key;
    while (iss >> token)
    {
        args.push_back(token);
    }
}

void MessageParser::feed(const std::string &data)
{
    _buffer += data;
    size_t pos = _buffer.find('\n');
    std::string cmd;
    std::vector<std::string> args;
    std::string line;

    while (pos != std::string::npos)
    {
        line = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 1);
        if (!line.empty())
        {
            tokeniseLine(line, cmd, args);
            _registry.dispatch(cmd, args, _state);
        }
        pos = _buffer.find('\n');
    }
}

} // namespace Zappy
