#pragma once
#include <string>
#include "Teams.hpp"
#include "../../ServerException/ServerException.hpp"

namespace zappy
{
    struct position
    {
        int x;
        int y;
    };

    class Player
    {
        private:
            int _fd; // Needed to link the player to its client
            
            int _playerID;
            int _level = 1;
            position _pos;
            int rotation;
            Team _team;
            bool isLeveling;
        public:
            Player(int playerID, int fd, const Team &team) : _fd(fd), _playerID(playerID), _pos{0, 0}, _team(team), isLeveling(false) {}
            int getPlayerID() const;
            const position &getPosition() const;
            int getRotation() const;
            int getLevel() const;

            void setRotation(int rotation);
            void setPosition(int x, int y, std::pair<int, int> mapSize);
            void move(std::pair<int, int> mapSize);
            void levelUp();
    };
} // namespace zappy
