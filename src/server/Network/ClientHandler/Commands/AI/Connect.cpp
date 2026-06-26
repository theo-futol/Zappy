#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients)
{
    (void)args; // Unused parameter
    (void)clients;
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
        return "ko\n";
    int count = player->getTeam().getAvailableSlots();
    Logger::log("01C", "Connect_nbr : response sent", {{"player_id", std::to_string(player->getId())}, {"count", std::to_string(count)}});
    return std::to_string(count) + "\n";
}
} // namespace zappy
