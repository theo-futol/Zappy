#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Sst(std::vector<std::string> args, Client &client)
{
    (void)client; // Unused parameter
    try
    {
        if (args.size() < 1)
            return "ko\n";
        int newF = std::stoi(args[0]);
        if (newF < 1 || newF > 1000)
            return "ko\n";
        _world->setTimeUnit(newF);
        std::string response = "sst " + std::to_string(newF) + "\n";
        send(client.getFd(), response.c_str(), response.size(), MSG_NOSIGNAL);
    }
    catch (...)
    {
        return "ko\n";
    }
    return "";
}
} // namespace zappy
