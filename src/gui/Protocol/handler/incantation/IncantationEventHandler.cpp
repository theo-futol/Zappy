#include "Protocol/handler/incantation/IncantationEventHandler.hpp"

#include <exception>

namespace Zappy
{

std::vector<std::string> IncantationEventHandler::keys() const
{
    return {"pic", "pie"};
}

void IncantationEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    // pic X Y L #n...  -> incantation starts on tile (X, Y).
    // pie X Y R        -> incantation ends on tile (X, Y) with result R.
    if (args.size() < 2)
        return;
    try
    {
        int x = std::stoi(args[0]);
        int y = std::stoi(args[1]);

        if (x < 0 || y < 0 || x >= state.map().width() || y >= state.map().height())
            return;
        state.map().at(x, y).setIncanting(key == "pic");
    }
    catch (const std::exception &)
    {
        // Malformed coordinates: ignore the event rather than crash the GUI.
    }
}

} // namespace Zappy
