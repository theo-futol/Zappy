#include "types/EntityKey.hpp"

namespace Zappy
{

bool EntityKey::operator<(const EntityKey &other) const
{
    if (entityType != other.entityType)
        return entityType < other.entityType;
    return number < other.number;
}

} // namespace Zappy
