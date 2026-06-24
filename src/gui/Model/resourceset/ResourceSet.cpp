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

} // namespace Zappy
