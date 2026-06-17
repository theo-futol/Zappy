#include "../../../Simulation/World/World.hpp"
#include "../../Client/Client.hpp"
#include <queue>

namespace zappy
{
    class Commands
    {
        private:
            World *_world;
            std::queue<std::string> *_broadcastQueue;
        public:
            Commands(World *world, std::queue<std::string> *broadcastQueue) : _world(world), _broadcastQueue(broadcastQueue) {}
            ~Commands() = default;
            // AI Commands
            std::string Forward(std::vector<std::string> args, Client &client);
            std::string Right(std::vector<std::string> args, Client &client);
            std::string Left(std::vector<std::string> args, Client &client);
            std::string Look(std::vector<std::string> args, Client &client);
            std::string getInventory(std::vector<std::string> args, Client &client);
            std::string Connect_nbr(std::vector<std::string> args, Client &client);
            std::string Broadcast(std::vector<std::string> args, Client &client);
            std::string Eject(std::vector<std::string> args, Client &client);
            std::string Take(std::vector<std::string> args, Client &client);
            std::string Set(std::vector<std::string> args, Client &client);
            std::string Fork(std::vector<std::string> args, Client &client);
            std::string Incantation(std::vector<std::string> args, Client &client);
            // Graphic Commands
            std::string Ppo(std::vector<std::string> args, Client &client);
            std::string Plv(std::vector<std::string> args, Client &client);
            std::string Pin(std::vector<std::string> args, Client &client);
            std::string Sgt(std::vector<std::string> args, Client &client);
            std::string Sst(std::vector<std::string> args, Client &client);
    };
}
