#include <gtest/gtest.h>

#include <antwika/event/TickEvent.hpp>

#include "antwika/engine/Events.hpp"
#include "antwika/engine/StopSignal.hpp"

using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::TickEvent;

TEST(StopSignalTest, Stopped_IsFalseBeforeAnyEventIsHandled)
{
    StopSignal stopSignal;

    EXPECT_FALSE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_StaysFalseForUnrelatedEvents)
{
    StopSignal stopSignal;

    stopSignal.handle(
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}});
    stopSignal.handle(
        TickEvent{.tick = 1, .event = Event{.name = "game.score_increment"}});

    EXPECT_FALSE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_BecomesTrueOnceStopIsHandled)
{
    StopSignal stopSignal;

    stopSignal.handle(TickEvent{
        .tick = 2,
        .event = Event{.name = antwika::engine::events::kStop}});

    EXPECT_TRUE(stopSignal.stopped());
}

TEST(StopSignalTest, Stopped_StaysTrueAfterFurtherEventsAreHandled)
{
    StopSignal stopSignal;

    stopSignal.handle(TickEvent{
        .tick = 2,
        .event = Event{.name = antwika::engine::events::kStop}});
    stopSignal.handle(
        TickEvent{.tick = 3, .event = Event{.name = "engine.tick"}});

    EXPECT_TRUE(stopSignal.stopped());
}
