#include "Core/Core.hpp"
#include "ServerException/ServerException.hpp"

int main(int ac, char **av)
{
    try
    {
        std::cout << "Starting Zappy server..." << std::endl;
        zappy::ArgParser argParser(ac, av);
        zappy::Core core(argParser);
        std::cout << "Setting up the world..." << std::endl;
        core.setWorld();
        std::cout << "Setting up the network..." << std::endl;
        core.setClientHandler();
        std::cout << "Server is up and running." << std::endl;
        core.run();
    }
    catch (const zappy::ServerException &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}