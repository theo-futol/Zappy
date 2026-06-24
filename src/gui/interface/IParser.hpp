#pragma once

#include <string>

namespace Zappy
{

/**
 * @class IParser
 * @brief Contract for consuming the raw server byte stream.
 */
class IParser
{
  public:
    virtual ~IParser() = default;

    /**
     * @brief Feeds a chunk of raw data, dispatching any complete messages found.
     * @param data Newly received bytes.
     */
    virtual void feed(const std::string &data) = 0;
};

} // namespace Zappy
