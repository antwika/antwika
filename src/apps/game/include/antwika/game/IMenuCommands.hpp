#pragma once

namespace antwika::game
{

    /**
     * @brief What the toolbar's game menu can ask the session to do.
     *
     * The verbs behind MenuItem, and nothing else. It exists so that
     * UiSink -- whose subject is a layout and a pointer -- does not have
     * to hold a World, a save store and a world map just to act on a
     * list it drew, and so that what an item *does* can be asserted
     * apart from where that item happens to sit.
     *
     * **None of these is an event and none may ever become one.** Each
     * is a consequence of a recorded click, applied inside the tick path
     * exactly as a placement is, so a replay re-derives it rather than
     * being told about it -- see Events.hpp.
     *
     * Save and load share one verb because they share one screen: the
     * picker is where a session is both written out and read back, and
     * two verbs asking for it would be one thing said twice.
     */
    class IMenuCommands
    {
    public:
        virtual ~IMenuCommands() = default;

        /**
         * @brief Start again on an empty grid.
         *
         * Empties the city that is open, puts its camera back where a
         * run starts it and resets the plain state the reducer folds.
         * The other cities of a world keep what was built on them, since
         * a session holds one live grid -- see WorldMapState.
         */
        virtual void newGame() = 0;

        /**
         * @brief Show the picker a session is saved to and loaded from.
         */
        virtual void openSaves() = 0;

        /**
         * @brief Leave the city for the main menu.
         */
        virtual void mainMenu() = 0;

        /**
         * @brief Put the city away and show the world map.
         */
        virtual void worldMap() = 0;
    };

} // namespace antwika::game
