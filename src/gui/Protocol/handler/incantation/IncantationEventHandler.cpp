#include "Protocol/handler/incantation/IncantationEventHandler.hpp"

namespace Zappy
{

std::vector<std::string> IncantationEventHandler::keys() const
{
    return {"pic", "pie"};
}

void IncantationEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    (void)key;
    (void)args;
    (void)state;
    // pic/pie are purely visual events (incantation animation on a tile). The v1 model has
    // no incantation state, and resource changes from a successful ritual arrive via bct.
    // This handler is the documented seam: the render phase will add the model support it needs.
}

} // namespace Zappy
