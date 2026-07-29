#include <gtest/gtest.h>

#include <antwika/event/TimedEvent.hpp>

#include "antwika/engine/Events.hpp"
#include "antwika/engine/StopSignal.hpp"

using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::TimedEvent;

TEST(StopSignalTest, Stopped_IsFalseBeforeAnyEventIsHandled)
{
    StopSignal stopSignal;

    EXPECT_FALSE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_StaysFalseForUnrelatedEvents)
{
    StopSignal stopSignal;

    stopSignal.handle(
        TimedEvent{.tick = 0, .event = Event{.name = "engine.tick"}});
    stopSignal.handle(
        TimedEvent{.tick = 1, .event = Event{.name = "game.score_increment"}});

    EXPECT_FALSE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_BecomesTrueOnceStopIsHandled)
{
    StopSignal stopSignal;

    stopSignal.handle(TimedEvent{
        .tick = 2,
        .event = Event{.name = antwika::engine::events::kStop}});

    EXPECT_TRUE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_StaysTrueAfterFurtherEventsAreHandled)
{
    StopSignal stopSignal;

    stopSignal.handle(TimedEvent{
        .tick = 2,
        .event = Event{.name = antwika::engine::events::kStop}});
    stopSignal.handle(
        TimedEvent{.tick = 3, .event = Event{.name = "engine.tick"}});

    EXPECT_TRUE(stopSignal.stopped());
}
