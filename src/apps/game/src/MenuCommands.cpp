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
        SaveGame fresh;
        fresh.camera = home;

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
        cities.closeCity(live);
        mode.request(AppMode::WorldMap);
    }

}
