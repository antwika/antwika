#include "antwika/game/Game.hpp"

#include <antwika/engine/Engine.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/log/Logger.hpp>

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::log::Logger;

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
        engine.step(0);
    }

    void bootstrap(IClock &clock,
                    IAppender &appender,
                    IFormatter &formatter,
                    ILogPolicy &logPolicy,
                    IEventQueue &eventQueue,
                    IEventSink &eventSink)
    {
        Logger logger(formatter, logPolicy, clock, appender);
        EventDispatcher dispatcher(eventQueue, {eventSink});
        Engine engine(logger, eventQueue, dispatcher);
        Game game(engine, dispatcher);

        game.run();
    }

} // namespace antwika::game
