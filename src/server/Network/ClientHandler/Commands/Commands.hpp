#include "../../../Simulation/World/World.hpp"
#include "../../Client/Client.hpp"

namespace zappy
{
    class Commands
    {
        private:
            World *_world;
        public:
            Commands(World *world) : _world(world) {}
            ~Commands() = default;
            // AI Commands
            std::string Forward(std::string command, Client &client);
            std::string Right(std::string command, Client &client);
            std::string Left(std::string command, Client &client);
            std::string Look(std::string command, Client &client);
            std::string Inventory(std::string command, Client &client);
            std::string Connect_nbr(std::string command, Client &client);
            std::string Broadcast(std::string command, Client &client);
            std::string Eject(std::string command, Client &client);
            std::string Take(std::string command, Client &client);
            std::string Set(std::string command, Client &client);
            std::string Fork(std::string command, Client &client);
            std::string Incantation(std::string command, Client &client);
            // Graphic Commands
            std::string Ppo(std::string command, Client &client);
            std::string Plv(std::string command, Client &client);
            std::string Pin(std::string command, Client &client);
            std::string Sgt(std::string command, Client &client);
            std::string Sst(std::string command, Client &client);
    };
}
