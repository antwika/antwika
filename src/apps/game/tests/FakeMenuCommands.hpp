#pragma once

#include <cstddef>

#include "antwika/game/IMenuCommands.hpp"

namespace antwika::game::tests
{

    class FakeMenuCommands final : public antwika::game::IMenuCommands
    {
    public:
        void newGame() override
        {
            ++newGames;
        }

        void openSaves() override
        {
            ++saves;
        }

        void mainMenu() override
        {
            ++mainMenus;
        }

        void worldMap() override
        {
            ++worldMaps;
        }

        std::size_t newGames = 0;
        std::size_t saves = 0;
        std::size_t mainMenus = 0;
        std::size_t worldMaps = 0;
    };

}
