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
            // Commands interface methods
            std::string Forward(std::string command, Client &client);
            std::string Right(std::string command, Client &client);
            std::string Left(std::string command, Client &client);
    };
}
