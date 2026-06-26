#include "ArgParser.hpp"
#include "../ServerException/ServerException.hpp"

namespace zappy
{
ArgParser::ArgParser(int argc, char **argv) : _argc(argc), _argv(argv)
{
}

void ArgParser::registerFlag(const std::string &flag, FlagType type, bool required, const std::string &description)
{
    _flagConfigs[flag] = {type, required, description};
}

void ArgParser::parse()
{
    for (int i = 1; i < _argc; i++)
    {
        std::string token(_argv[i]);
        if (token.empty() || token[0] != '-')
            throw ServerException("Unexpected value without flag: " + token);
        auto it = _flagConfigs.find(token);
        if (it == _flagConfigs.end())
            throw ServerException("Unknown flag: " + token);
        const FlagConfig &cfg = it->second;
        if (cfg.type == FlagType::FLAG)
        {
            _parsedValues[token];
        }
        else if (cfg.type == FlagType::LIST)
        {
            while (i + 1 < _argc && _argv[i + 1][0] != '-')
                _parsedValues[token].push_back(_argv[++i]);
            if (_parsedValues[token].empty())
                throw ServerException("Flag " + token + " requires at least one value");
        }
        else
        {
            if (i + 1 >= _argc || _argv[i + 1][0] == '-')
                throw ServerException("Flag " + token + " requires a value");
            _parsedValues[token].push_back(_argv[++i]);
        }
    }

    std::string missing;
    for (const auto &[flag, cfg] : _flagConfigs)
        if (cfg.required && _parsedValues.find(flag) == _parsedValues.end())
            missing += "  " + flag + " : " + cfg.description + "\n";
    if (!missing.empty())
        throw ServerException("Missing required flags:\n" + missing);
}

bool ArgParser::hasFlag(const std::string &flag) const
{
    return _parsedValues.find(flag) != _parsedValues.end();
}

int ArgParser::getInt(const std::string &flag) const
{
    const auto &values = getList(flag);
    try
    {
        return std::stoi(values[0]);
    }
    catch (...)
    {
        throw ServerException("Flag " + flag + " value is not a valid integer: " + values[0]);
    }
}

std::string ArgParser::getString(const std::string &flag) const
{
    return getList(flag)[0];
}

std::vector<std::string> ArgParser::getList(const std::string &flag) const
{
    auto it = _parsedValues.find(flag);
    if (it == _parsedValues.end())
        throw ServerException("Flag " + flag + " was not parsed");
    return it->second;
}

} // namespace zappy
