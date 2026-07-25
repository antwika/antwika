#include "Game.hpp"

#include <iostream>
#include <antwika/time/SystemClock.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/Event.hpp>

namespace antwika::game
{

    void Game::run()
    {
        antwika::time::SystemClock clock;
        antwika::log::StreamAppender appender(std::cout);
        antwika::log::Logger logger(clock, antwika::log::Level::Info, appender);

        antwika::event::EventRecorder eventRecorder;
        antwika::event::EventQueue eventQueue(eventRecorder);

        antwika::engine::Engine engine(logger, eventQueue);

        eventQueue.enqueue(antwika::event::Event{.name = "ExampleEvent"});

        engine.start();
    }

} // namespace antwika::game
