#include <iostream>

#include "Core/Core.hpp"

int main(int argc, char **argv)
{
    try
    {
        Zappy::Core core(argc, argv);
        core.init();
        core.run();
    }
    catch (const Zappy::Core::CoreException &error)
    {
        std::cerr << error.what() << std::endl;
        return 84;
    }
    return 0;
}
