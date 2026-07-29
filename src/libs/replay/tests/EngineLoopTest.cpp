#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/engine/mocks/MockEngine.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/event/mocks/MockEventDispatcher.hpp>

#include "antwika/replay/EngineLoop.hpp"
#include "antwika/replay/EngineLoopError.hpp"
#include "antwika/replay/ReplaySource.hpp"

using antwika::engine::StopSignal;
using antwika::engine::mocks::MockEngine;
using antwika::event::Event;
using antwika::event::TickedEventDispatcher;
using antwika::event::TimedEvent;
using antwika::event::mocks::MockEventDispatcher;
using antwika::replay::EngineLoop;
using antwika::replay::EngineLoopError;
using antwika::replay::ReplaySource;

TEST(EngineLoopTest, Run_DispatchesSourcedEventsThenStepsEngineUntilStop)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({
        TimedEvent{.tick = 0, .event = Event{.name = "a"}},
        TimedEvent{.tick = 2, .event = Event{.name = "b"}},
        TimedEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = "a"}));
        EXPECT_CALL(mockEngine, step(0));
        EXPECT_CALL(mockEngine, step(1));
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = "b"}));
        EXPECT_CALL(
            mockEventDispatcher,
            dispatch(Event{.name = antwika::engine::events::kStop}));
        EXPECT_CALL(mockEngine, step(2));
    }

    loop.run(stopSignal, 10);
}

// Production callers omit maxTicks entirely to run uncapped.
// That must not throw, no matter how many ticks it takes to see a stop.
TEST(EngineLoopTest, Run_RunsUncappedUntilStopWhenMaxTicksIsOmitted)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({
        TimedEvent{
            .tick = 5,
            .event = Event{.name = antwika::engine::events::kStop}},
    });
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockEngine, step(0));
        EXPECT_CALL(mockEngine, step(1));
        EXPECT_CALL(mockEngine, step(2));
        EXPECT_CALL(mockEngine, step(3));
        EXPECT_CALL(mockEngine, step(4));
        EXPECT_CALL(mockEngine, step(5));
    }

    loop.run(stopSignal);
}

TEST(EngineLoopTest, Run_ThrowsWithoutSteppingWhenMaxTicksIsZero)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({});
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    EXPECT_CALL(mockEngine, step(::testing::_)).Times(0);

    EXPECT_THROW(loop.run(stopSignal, 0), EngineLoopError);
}

TEST(EngineLoopTest, Run_ThrowsAfterRunningMaxTicksWithoutAStopEvent)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({});
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockEngine, step(0));
        EXPECT_CALL(mockEngine, step(1));
        EXPECT_CALL(mockEngine, step(2));
    }

    EXPECT_THROW(loop.run(stopSignal, 3), EngineLoopError);
}
