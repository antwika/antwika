#include "antwika/game/Game.hpp"

#include <antwika/engine/Engine.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/game/GameStateReducer.hpp"

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Logger;
using antwika::replay::EngineLoop;

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

    GameState bootstrap(IClock &clock,
                        IAppender &appender,
                        IFormatter &formatter,
                        ILogPolicy &logPolicy,
                        IEventQueue &eventQueue,
                        IEventSink &eventSink,
                        IReplaySource &inputSource,
                        antwika::time::Tick totalTicks)
    {
        Logger logger(formatter, logPolicy, clock, appender);
        EventDispatcher dispatcher(eventQueue, {eventSink});

        GameState state;
        GameStateReducer reducer(state);
        TickedEventDispatcher tickedDispatcher(dispatcher, {reducer});

        Engine engine(logger, eventQueue, tickedDispatcher);
        Game game(engine, tickedDispatcher);
        game.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(totalTicks);

        return state;
    }

} // namespace antwika::game
