#include <gtest/gtest.h>

#include "antwika/event/TickEvent.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;

TEST(TickEventTest, Ctor_DefaultsToTickZeroAndAnEmptyEvent)
{
    TickEvent timedEvent;
    EXPECT_EQ(timedEvent.tick, 0);
    EXPECT_EQ(timedEvent.event, Event{});
}

TEST(TickEventTest, Ctor_TakesATickAndEvent)
{
    TickEvent timedEvent{.tick = 7, .event = Event{.name = "Event"}};
    EXPECT_EQ(timedEvent.tick, 7);
    EXPECT_EQ(timedEvent.event.name, "Event");
}

TEST(TickEventTest, OperatorEquals_MatchesOnEveryField)
{
    TickEvent timedEvent1{.tick = 1, .event = Event{.name = "Event"}};
    TickEvent timedEvent2{.tick = 1, .event = Event{.name = "Event"}};
    EXPECT_EQ(timedEvent1, timedEvent2);
}

TEST(TickEventTest, OperatorEquals_SeparatesDifferentTicks)
{
    TickEvent timedEvent1{.tick = 1, .event = Event{.name = "Event"}};
    TickEvent timedEvent2{.tick = 2, .event = Event{.name = "Event"}};
    EXPECT_NE(timedEvent1, timedEvent2);
}

TEST(TickEventTest, OperatorEquals_SeparatesDifferentEvents)
{
    TickEvent timedEvent1{.tick = 1, .event = Event{.name = "Event 1"}};
    TickEvent timedEvent2{.tick = 1, .event = Event{.name = "Event 2"}};
    EXPECT_NE(timedEvent1, timedEvent2);
}
