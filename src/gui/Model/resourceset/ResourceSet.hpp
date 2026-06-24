#pragma once

#include <array>
#include <cstddef>

#include "types/ResourceType.hpp"

namespace Zappy
{

/**
 * @class ResourceSet
 * @brief A bag of the seven resource counts.
 *
 * Shared by Tile (resources on the ground) and Trantorian (carried inventory).
 */
class ResourceSet
{
  public:
    /// @brief Number of distinct resource types.
    static constexpr std::size_t Count = 7;

    ResourceSet();

    /**
     * @brief Returns the count of a resource.
     * @param type Resource to query.
     * @return The stored count.
     */
    int get(ResourceType type) const;

    /**
     * @brief Sets the count of a resource.
     * @param type Resource to set.
     * @param value New count.
     */
    void set(ResourceType type, int value);

  private:
    std::array<int, Count> _counts; ///< Per-resource counts indexed by ResourceType.
};

} // namespace Zappy
