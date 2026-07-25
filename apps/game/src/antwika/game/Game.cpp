#include "Game.hpp"

#include <iostream>
#include <antwika/time/SystemClock.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/Event.hpp>

namespace antwika::game
{

    void Game::run()
    {
        antwika::time::SystemClock clock;
        antwika::log::StreamAppender appender(std::cout);
        antwika::log::PlainFormatter formatter;

        antwika::log::MinimumLevelLogPolicy logPolicy(antwika::log::Level::Info);
        antwika::log::Logger logger(formatter, logPolicy, clock, appender);

        antwika::event::EventRecorder eventRecorder;
        antwika::event::EventQueue eventQueue;

        antwika::event::EventDispatcher eventDispatcher(eventQueue, {eventRecorder});

        antwika::engine::Engine engine(logger, eventQueue);

        eventDispatcher.dispatch(antwika::event::Event{.name = "ExampleEvent"});

        engine.start();
    }

} // namespace antwika::game
