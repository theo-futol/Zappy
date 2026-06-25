#pragma once
#include <string>
#include <utility>
#include <vector>

namespace zappy
{
    /// @brief Emits server log lines following the protocol's response grammar:
    ///        "code - message (key=value, key=value)".
    class Logger
    {
    public:
        using Field = std::pair<std::string, std::string>;

        /// @brief Prints one log line to stdout. Omits the "(...)" group when fields is empty.
        static void log(const std::string &code, const std::string &message, const std::vector<Field> &fields = {});
    };
} // namespace zappy
