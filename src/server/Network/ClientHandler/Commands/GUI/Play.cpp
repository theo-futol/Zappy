/* ------------------------------------------------------------------------------------ *
 *                                                                                      *
 * EPITECH PROJECT - Wed, Jul, 2026                                                     *
 * Title           - zappy                                                              *
 * Description     -                                                                    *
 *     Play                                                                             *
 * ------------------------------------------------------------------------------------ */

#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Play(std::vector<std::string>, Client &)
{
    if (!_world->isPaused())
        return "ko\n";
    _world->setPaused(false);
    return "ok\n";
}
} // namespace zappy
