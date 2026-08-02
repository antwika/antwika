#pragma once

#include <cstddef>

#include "antwika/game/IMenuCommands.hpp"

namespace antwika::game::tests
{

    /**
     * @brief Counts what the game menu asked for, and does none of it.
     *
     * What an item *does* is MenuCommandsTest's subject, against the
     * real session and the real world map. What a fixture holding this
     * is about is which item a click at a pixel resolves to, which is
     * the half that depends on a layout -- so the two are asserted
     * apart and neither drags the other's collaborators in.
     */
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

} // namespace antwika::game::tests
