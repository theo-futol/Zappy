#include "abstract/AEventHandler.hpp"

#include <sstream>

#include "types/ResourceType.hpp"

namespace Zappy
{

int AEventHandler::toInt(const std::string &token)
{
    std::istringstream iss(token);
    int value = 0;

    iss >> value;
    return value;
}

int AEventHandler::entityNumber(const std::string &token)
{
    if (!token.empty() && token.front() == '#')
        return toInt(token.substr(1));
    return toInt(token);
}

Orientation AEventHandler::toOrientation(int value)
{
    if (value >= 1 && value <= 4)
        return static_cast<Orientation>(value);
    return Orientation::North;
}

ResourceSet AEventHandler::toResources(const std::vector<std::string> &args, std::size_t offset)
{
    ResourceSet resources;

    for (std::size_t i = 0; i < ResourceSet::Count; ++i)
    {
        std::size_t index = offset + i;

        if (index < args.size())
            resources.set(static_cast<ResourceType>(i), toInt(args[index]));
    }
    return resources;
}

} // namespace Zappy
