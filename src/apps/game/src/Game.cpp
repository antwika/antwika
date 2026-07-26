#include "antwika/game/Game.hpp"

namespace antwika::game
{

    Game::Game(IEngine &engine,
               IEventDispatcher &dispatcher) : engine(engine),
                                               eventDispatcher(eventDispatcher)
    {
    }

    void Game::run()
    {
        engine.start();
    }

} // namespace antwika::game
