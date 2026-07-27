#include <gtest/gtest.h>

#include "antwika/event/TimedEvent.hpp"

using antwika::event::Event;
using antwika::event::TimedEvent;

TEST(TimedEventTest, DefaultConstruction)
{
    TimedEvent timedEvent;
    EXPECT_EQ(timedEvent.tick, 0);
    EXPECT_EQ(timedEvent.event, Event{});
}

TEST(TimedEventTest, AggregateConstruction)
{
    TimedEvent timedEvent{.tick = 7, .event = Event{.name = "Event"}};
    EXPECT_EQ(timedEvent.tick, 7);
    EXPECT_EQ(timedEvent.event.name, "Event");
}

TEST(TimedEventTest, Equality)
{
    TimedEvent timedEvent1{.tick = 1, .event = Event{.name = "Event"}};
    TimedEvent timedEvent2{.tick = 1, .event = Event{.name = "Event"}};
    EXPECT_EQ(timedEvent1, timedEvent2);
}

TEST(TimedEventTest, InequalityWhenOnlyTickDiffers)
{
    TimedEvent timedEvent1{.tick = 1, .event = Event{.name = "Event"}};
    TimedEvent timedEvent2{.tick = 2, .event = Event{.name = "Event"}};
    EXPECT_NE(timedEvent1, timedEvent2);
}

TEST(TimedEventTest, InequalityWhenOnlyEventDiffers)
{
    TimedEvent timedEvent1{.tick = 1, .event = Event{.name = "Event 1"}};
    TimedEvent timedEvent2{.tick = 1, .event = Event{.name = "Event 2"}};
    EXPECT_NE(timedEvent1, timedEvent2);
}
