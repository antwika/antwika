#include "antwika/game/Game.hpp"

#include <antwika/engine/Engine.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventDispatcher.hpp>

#include <iostream>

using antwika::engine::Engine;
using antwika::event::EventDispatcher;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::game::Game;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::time::SystemClock;

int main()
{
    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);
    EventRecorder eventRecorder;
    EventQueue eventQueue;
    EventDispatcher eventDispatcher(eventQueue, {eventRecorder});
    Engine engine(logger, eventQueue);

    eventDispatcher.dispatch(antwika::event::Event{.name = "ExampleEvent"});

    Game game(engine, eventDispatcher);

    game.run();
}
