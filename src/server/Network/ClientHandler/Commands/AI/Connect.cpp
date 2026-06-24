#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    int count = player->getTeam().getAvailableSlots();
    Logger::log("01C", "Connect_nbr : response sent", {{"player_id", std::to_string(player->getFd())}, {"count", std::to_string(count)}});
    return std::to_string(count) + "\n";
}
} // namespace zappy
