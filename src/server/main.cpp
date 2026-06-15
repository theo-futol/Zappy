#include "Core/Core.hpp"
#include "ServerException/ServerException.hpp"

int main(int ac, char **av)
{
    try
    {
        zappy::ArgParser argParser(ac, av);
        zappy::Core core(argParser);

        core.setWorld();
        core.setClientHandler();
        core.run();
    }
    catch (const zappy::ServerException &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}