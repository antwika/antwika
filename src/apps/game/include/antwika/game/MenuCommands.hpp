#pragma once

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/IMenuCommands.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/Tuning.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    /**
     * @brief The one implementation of the game menu's verbs.
     *
     * Every one of them is something another route already reaches:
     * leaving for the main menu is what the menu modal's own item does,
     * showing the picker is what the main menu's Load Game does, and
     * putting the city away is what the world-map key does. Writing them
     * once here is what keeps the menu from being a second set of rules
     * about the same four transitions.
     *
     * A mode change is *requested* rather than applied, so it lands at
     * the tick boundary and the click that asked for it cannot also be
     * read by the screen it asks for -- see AppMode.hpp.
     */
    class MenuCommands final : public IMenuCommands
    {
    public:
        /**
         * @brief Construct the verbs over what they act on.
         * @param mode The app's mode, asked for a screen. Must outlive
         * this object.
         * @param session The one route between the live grid and a save;
         * a new game restores an empty one through it. Must outlive this
         * object.
         * @param cities Which city is open, put away by worldMap(). Must
         * outlive this object.
         * @param live The grid a session builds on, kept with the city
         * it is put away with. Must outlive this object.
         * @param home Where a new game's camera is put back to; a copy,
         * since it is a value the run was configured with rather than a
         * camera anybody moves.
         * @param tuning Where a new game's bank opens; a copy, on the
         * same terms the camera is.
         */
        MenuCommands(
            AppModeState &mode,
            SessionStore &session,
            WorldMapState &cities,
            const LiveGrid &live,
            Camera home,
            Tuning tuning);

        MenuCommands(const MenuCommands &) = delete;
        MenuCommands(MenuCommands &&) = delete;

        MenuCommands &operator=(const MenuCommands &) = delete;
        MenuCommands &operator=(MenuCommands &&) = delete;

        /**
         * @brief Start again on an empty grid.
         */
        void newGame() override;

        /**
         * @brief Show the picker a session is saved to and loaded from.
         */
        void openSaves() override;

        /**
         * @brief Leave the city for the main menu.
         */
        void mainMenu() override;

        /**
         * @brief Put the city away and show the world map.
         */
        void worldMap() override;

    private:
        AppModeState &mode;
        SessionStore &session;
        WorldMapState &cities;
        const LiveGrid &live;
        Camera home;
        Tuning tuning;
    };

} // namespace antwika::game
