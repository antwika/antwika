#include "antwika/game/Game.hpp"

#include <antwika/event/Event.hpp>

using antwika::event::Event;

namespace antwika::game
{

    Game::Game(IEngine &engine,
               IEventDispatcher &dispatcher) : engine(engine),
                                               dispatcher(dispatcher)
    {
    }

    void Game::run()
    {
        dispatcher.dispatch(Event{.name = "Running Antwika Game"});
        engine.start();
    }

} // namespace antwika::game
