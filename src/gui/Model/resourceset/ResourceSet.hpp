#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

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

    /**
     * @brief One "Name: count" line per resource, for display in a HUD bubble.
     * @param skipEmpty When true, resources with a count of 0 are omitted.
     * @return The description lines (empty if skipEmpty and nothing is present).
     */
    std::vector<std::string> describe(bool skipEmpty) const;

  private:
    std::array<int, Count> _counts; ///< Per-resource counts indexed by ResourceType.
};

} // namespace Zappy
