/* ------------------------------------------------------------------------------------ *
 *                                                                                      *
 * EPITECH PROJECT - Wed, Jul, 2026                                                     *
 * Title           - zappy                                                              *
 * Description     -                                                                    *
 *     Stop                                                                             *
 * ------------------------------------------------------------------------------------ */

#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Stop(std::vector<std::string>, Client &)
{
    _world->setPaused(true);
    return "ok\n";
}
} // namespace zappy
