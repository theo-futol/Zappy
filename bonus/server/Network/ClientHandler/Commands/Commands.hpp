#pragma once
#include "../../../Simulation/World/World.hpp"
#include "../../Client/Client.hpp"
#include <chrono>
#include <memory>
#include <queue>

namespace zappy
{
    /// @brief Implements every protocol command's effect on the world.
    ///
    /// One shared instance backs all clients. Each method applies a command for the
    /// given client and returns the reply string to send back (AI commands), or pushes
    /// updates onto the broadcast queue (graphic commands). The method names mirror the
    /// zappy protocol verbs.
    class Commands
    {
        private:
            World *_world;                            ///< Shared world the commands act on (not owned).
            std::queue<std::string> *_broadcastQueue; ///< Queue of updates destined for graphic clients (not owned).

            /// @brief Parses the "#n" (or bare "n") player-id token of a graphic command; -1 on failure.
            static int parsePlayerIdArg(const std::vector<std::string> &args)
            {
                if (args.empty())
                    return -1;
                std::string token = args[0];
                if (!token.empty() && token[0] == '#')
                    token = token.substr(1);
                try
                {
                    return std::stoi(token);
                }
                catch (...)
                {
                    return -1;
                }
            }
        public:
            Commands(World *world, std::queue<std::string> *broadcastQueue) : _world(world), _broadcastQueue(broadcastQueue) {}
            ~Commands() = default;

            // AI Commands
            /// @brief Moves the player one tile forward in its current facing.
            std::string Forward(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Turns the player 90° to the right.
            std::string Right(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Turns the player 90° to the left.
            std::string Left(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Returns what the player sees on the tiles within its vision cone.
            std::string Look(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Returns the player's inventory (resource counts and remaining life).
            std::string getInventory(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Returns the number of free team connection slots (the "Connect_nbr" command).
            std::string Connect_nbr(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Broadcasts text to every other player, tagged with the direction it came from.
            std::string Broadcast(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Pushes every other player off the current tile.
            std::string Eject(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Picks up a named resource from the current tile into the inventory.
            std::string Take(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Drops a named resource from the inventory onto the current tile.
            std::string Set(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Lays an egg on the current tile, opening a new team slot.
            std::string Fork(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Attempts to start an elevation ritual on the current tile (see beginIncantation).
            std::string Incantation(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients);
            /// @brief Starts an incantation: validates prerequisites, notifies the initiator and the GUI,
            ///        and freezes every participant until `endTime`. Returns false (and replies "ko") if the
            ///        prerequisites are not met, in which case the ritual must not be scheduled.
            bool beginIncantation(Client &client, std::chrono::steady_clock::time_point endTime, std::vector<std::unique_ptr<Client>> &clients);

            // Graphic Commands
            /// @brief ppo: reports a player's position and orientation.
            std::string Ppo(std::vector<std::string> args, Client &client);
            /// @brief plv: reports a player's level.
            std::string Plv(std::vector<std::string> args, Client &client);
            /// @brief pin: reports a player's position and inventory.
            std::string Pin(std::vector<std::string> args, Client &client);
            /// @brief sgt: reports the current time unit (ticks per second).
            std::string Sgt(std::vector<std::string> args, Client &client);
            /// @brief sst: sets the time unit (ticks per second).
            std::string Sst(std::vector<std::string> args, Client &client);
            /// @brief msz: reports the map dimensions.
            std::string Msz(std::vector<std::string> args, Client &client);
            /// @brief bct: reports the content of a single tile.
            std::string Bct(std::vector<std::string> args, Client &client);
            /// @brief mct: reports the content of every tile on the map.
            std::string Mct(std::vector<std::string> args, Client &client);
            /// @brief tna: reports the name of every team.
            std::string Tna(std::vector<std::string> args, Client &client);
            /// @brief Builds a passive "pipi" snapshot line (position, orientation, level, inventory) for one player.
            std::string buildPipiMessage(Player &player) const;
    };
}
