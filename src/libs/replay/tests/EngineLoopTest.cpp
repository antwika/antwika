#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/mocks/MockEngine.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/event/mocks/MockEventDispatcher.hpp>

#include "antwika/replay/EngineLoop.hpp"
#include "antwika/replay/ReplaySource.hpp"

using antwika::engine::mocks::MockEngine;
using antwika::event::Event;
using antwika::event::TickedEventDispatcher;
using antwika::event::TimedEvent;
using antwika::event::mocks::MockEventDispatcher;
using antwika::replay::EngineLoop;
using antwika::replay::ReplaySource;

TEST(EngineLoopTest, Run_DispatchesSourcedEventsThenStepsEngineForEachTick)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    TickedEventDispatcher tickedEventDispatcher(mockEventDispatcher, {});
    ReplaySource source({
        TimedEvent{.tick = 0, .event = Event{.name = "a"}},
        TimedEvent{.tick = 2, .event = Event{.name = "b"}},
    });
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = "a"}));
        EXPECT_CALL(mockEngine, step(0));
        EXPECT_CALL(mockEngine, step(1));
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = "b"}));
        EXPECT_CALL(mockEngine, step(2));
    }

    loop.run(3);
}

TEST(EngineLoopTest, Run_DoesNothingForZeroTicks)
{
    MockEngine mockEngine;
    MockEventDispatcher mockEventDispatcher;
    TickedEventDispatcher tickedEventDispatcher(mockEventDispatcher, {});
    ReplaySource source({});
    EngineLoop loop(mockEngine, tickedEventDispatcher, source);

    EXPECT_CALL(mockEngine, step(::testing::_)).Times(0);

    loop.run(0);
}
