#include "Model/resourceset/ResourceSet.hpp"

namespace Zappy
{

ResourceSet::ResourceSet() : _counts{}
{
}

int ResourceSet::get(ResourceType type) const
{
    return _counts[static_cast<std::size_t>(type)];
}

void ResourceSet::set(ResourceType type, int value)
{
    _counts[static_cast<std::size_t>(type)] = value;
}

std::vector<std::string> ResourceSet::describe(bool skipEmpty) const
{
    std::vector<std::string> lines;

    for (std::size_t i = 0; i < Count; ++i)
    {
        ResourceType type = static_cast<ResourceType>(i);
        int count = _counts[i];

        if (skipEmpty && count <= 0)
            continue;
        lines.push_back(std::string(resourceName(type)) + ": " + std::to_string(count));
    }
    return lines;
}

} // namespace Zappy
