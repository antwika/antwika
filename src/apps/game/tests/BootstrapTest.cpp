#include <gtest/gtest.h>

#include <antwika/event/Event.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"

using antwika::event::Event;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::event::TimedEvent;
using antwika::game::GameState;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeClock;

// Bootstrap wires many already-unit-tested collaborators together (EventDispatcher, TickedEventDispatcher, Engine, EngineLoop, GameStateReducer, ...); re-asserting their exact internal call sequences here would be redundant with their own tests and brittle to boot.
// This test instead verifies the wiring end to end, black-box style: given a scripted input over a fixed number of ticks, does GameState come out right.
TEST(BootstrapTest, Bootstrap_RunsScriptedTicksAndReturnsResultingGameState)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventQueue eventQueue;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TimedEvent{.tick = 1, .event = Event{.name = antwika::game::events::kScoreIncrement, .payload = "5"}},
        TimedEvent{.tick = 3, .event = Event{.name = antwika::game::events::kScoreIncrement, .payload = "2"}},
    });

    auto state = antwika::game::bootstrap(fakeClock, appender, formatter, logPolicy, eventQueue, eventSink, inputSource, 5);

    EXPECT_EQ(state, (GameState{.ticksProcessed = 5, .score = 7}));
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputOnlyAdvancesTicks)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventQueue eventQueue;
    EventRecorder eventSink;

    ReplaySource inputSource({});

    auto state = antwika::game::bootstrap(fakeClock, appender, formatter, logPolicy, eventQueue, eventSink, inputSource, 3);

    EXPECT_EQ(state, (GameState{.ticksProcessed = 3, .score = 0}));
}
