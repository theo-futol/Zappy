#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Model/resourceset/ResourceSet.hpp"
#include "interface/ICommandHandler.hpp"
#include "types/Orientation.hpp"

namespace Zappy
{

/**
 * @class AEventHandler
 * @brief Abstract base for protocol handlers; factors the shared parsing helpers.
 *
 * Concrete handlers implement keys() and handle(); this base provides only static,
 * defensive parsing utilities, so the conversion logic lives in a single place (§3.2).
 * No helper throws: malformed input falls back to a safe default.
 */
class AEventHandler : public ICommandHandler
{
  protected:
    /**
     * @brief Parses an integer token, tolerant of malformed input.
     * @param token Token to convert.
     * @return The parsed value, or 0 if the token is not a valid integer.
     */
    static int toInt(const std::string &token);

    /**
     * @brief Parses an entity number token of the form "#n".
     * @param token Token to convert (a leading '#' is stripped if present).
     * @return The parsed number, or 0 if invalid.
     */
    static int entityNumber(const std::string &token);

    /**
     * @brief Converts a protocol orientation value to the Orientation enum.
     * @param value Protocol value (1=N, 2=E, 3=S, 4=W).
     * @return The matching orientation, or Orientation::North if out of range.
     */
    static Orientation toOrientation(int value);

    /**
     * @brief Reads the seven resource counts (q0..q6) starting at an offset.
     * @param args Argument list of the message.
     * @param offset Index of q0 within args.
     * @return The resource set; any missing value defaults to 0.
     */
    static ResourceSet toResources(const std::vector<std::string> &args, std::size_t offset);
};

} // namespace Zappy
