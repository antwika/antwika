#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/engine/mocks/MockEngine.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/event/mocks/MockEventDispatcher.hpp>
#include <antwika/event/mocks/MockTickEventSink.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/engine/EngineLoop.hpp"
#include "antwika/engine/EngineLoopError.hpp"

using antwika::engine::EngineLoop;
using antwika::engine::EngineLoopError;
using antwika::engine::StopSignal;
using antwika::engine::mocks::MockEngine;
using antwika::event::Event;
using antwika::event::TickedEventDispatcher;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventDispatcher;
using antwika::event::mocks::MockTickEventSink;
using antwika::replay::ReplaySource;
using ::testing::AnyNumber;

TEST(EngineLoopTest, Run_DispatchesSourcedEventsThenStepsEngineUntilStop)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
        TickEvent{.tick = 2, .event = Event{.name = "b"}},
        TickEvent{
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

TEST(EngineLoopTest, Run_StampsAnEventWithTheTickItArrivedOn)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    MockTickEventSink mockTickEventSink;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal, mockTickEventSink});
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
        TickEvent{.tick = 2, .event = Event{.name = "b"}},
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    EXPECT_CALL(mockEngine, step(::testing::_)).Times(AnyNumber());
    EXPECT_CALL(mockEventDispatcher, dispatch(::testing::_))
        .Times(AnyNumber());

    {
        ::testing::InSequence seq;
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{.tick = 0, .event = Event{.name = "a"}}));
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{.tick = 2, .event = Event{.name = "b"}}));
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{
                .tick = 2,
                .event = Event{.name = antwika::engine::events::kStop}}));
    }

    loop.run(stopSignal, 10);
}

TEST(EngineLoopTest, Run_RunsUncappedUntilStopWhenMaxTicksIsOmitted)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    StopSignal stopSignal;
    TickedEventDispatcher tickedEventDispatcher(
        mockEventDispatcher, {stopSignal});
    ReplaySource source({
        TickEvent{
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
