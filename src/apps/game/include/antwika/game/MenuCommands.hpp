#pragma once

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/IMenuCommands.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    class MenuCommands final : public IMenuCommands
    {
    public:
        MenuCommands(
            AppModeState &mode,
            SessionStore &session,
            WorldMapState &cities,
            const LiveGrid &live,
            Camera home,
            GameConfig config);

        MenuCommands(const MenuCommands &) = delete;
        MenuCommands(MenuCommands &&) = delete;

        MenuCommands &operator=(const MenuCommands &) = delete;
        MenuCommands &operator=(MenuCommands &&) = delete;

        void newGame() override;

        void openSaves() override;

        void mainMenu() override;

        void worldMap() override;

    private:
        AppModeState &mode;
        SessionStore &session;
        WorldMapState &cities;
        const LiveGrid &live;
        Camera home;
        GameConfig config;
    };

}
