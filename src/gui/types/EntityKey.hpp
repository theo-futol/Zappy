#pragma once

#include <string>

namespace Zappy
{

/**
 * @struct EntityKey
 * @brief Identity of a map entity: an open string entity type plus its protocol number.
 *
 * The entity type is a string (not an enum) so that an entity type introduced by the
 * server stays representable without modifying existing code. The pair
 * (entityType, number) keeps separate id spaces from colliding (e.g. player #0 and
 * egg #0 can coexist).
 */
struct EntityKey
{
    std::string entityType; ///< Open entity category (e.g. "player", "egg").
    int number;             ///< Protocol number within that category.

    /**
     * @brief Orders keys for use in an ordered associative container.
     * @param other Key compared against.
     * @return True if this key sorts strictly before @p other.
     */
    bool operator<(const EntityKey &other) const;
};

} // namespace Zappy
