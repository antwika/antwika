#include "antwika/game/Game.hpp"

#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/time/SystemClock.hpp>

#include <iostream>

using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::log::Level;
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
    EventQueue eventQueue;
    EventRecorder eventRecorder;

    antwika::game::bootstrap(clock, appender, formatter, logPolicy, eventQueue, eventRecorder);
}
