#include "Input/inputhandler/InputHandler.hpp"

namespace Zappy
{

InputHandler::InputHandler()
{
}

bool InputHandler::handle(const std::vector<Event> &events, TopDownCamera &camera)
{
    (void)events;
    (void)camera;
    return true;
}

} // namespace Zappy
