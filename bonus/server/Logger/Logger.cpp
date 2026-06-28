#include "Logger.hpp"
#include <iostream>

namespace zappy
{
void Logger::log(const std::string &code, const std::string &message, const std::vector<Field> &fields)
{
    std::cout << code << " - " << message;
    if (!fields.empty())
    {
        std::cout << " (";
        for (size_t i = 0; i < fields.size(); ++i)
        {
            if (i)
                std::cout << ", ";
            std::cout << fields[i].first << "=" << fields[i].second;
        }
        std::cout << ")";
    }
    std::cout << "\n";
}
} // namespace zappy
