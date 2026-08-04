#include "antwika/game/MenuCommands.hpp"

#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    MenuCommands::MenuCommands(
        AppModeState &mode,
        SessionStore &session,
        WorldMapState &cities,
        const LiveGrid &live,
        Camera home,
        GameConfig config)
        : mode(mode),
          session(session),
          cities(cities),
          live(live),
          home(home),
          config(config)
    {
    }

    void MenuCommands::newGame()
    {
        // Restored from an empty save rather than cleared by hand.
        // The store is the one route into the live grid.
        // A load destroys what is standing before it lays anything.
        // So an empty file is exactly "nothing is standing".
        // Any other way would be a second way to empty a city.
        SaveGame fresh;
        fresh.camera = home;

        // A fresh bank opens with the configured funds.
        // The save's own default is the shipped constant.
        fresh.state.money = config.startingMoney;

        session.restore(fresh);
    }

    void MenuCommands::openSaves()
    {
        mode.request(AppMode::SaveLoad);
    }

    void MenuCommands::mainMenu()
    {
        mode.request(AppMode::MainMenu);
    }

    void MenuCommands::worldMap()
    {
        // Put away first, asked for second, exactly as the key does.
        // The grid is kept with the city it belongs to.
        // And that has to happen before anything steps another one.
        cities.closeCity(live);
        mode.request(AppMode::WorldMap);
    }

} // namespace antwika::game
