#include <gtest/gtest.h>

#include "antwika/replay/ReplaySource.hpp"

using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::replay::ReplaySource;

TEST(ReplaySourceTest, EventsFor_ReturnsEventsRecordedAtThatTickInOrder)
{
    ReplaySource source({
        TimedEvent{.tick = 0, .event = Event{.name = "a"}},
        TimedEvent{.tick = 1, .event = Event{.name = "b"}},
        TimedEvent{.tick = 1, .event = Event{.name = "c"}},
    });

    auto tick1Events = source.eventsFor(1);

    ASSERT_EQ(tick1Events.size(), 2);
    EXPECT_EQ(tick1Events[0], (Event{.name = "b"}));
    EXPECT_EQ(tick1Events[1], (Event{.name = "c"}));
}

TEST(ReplaySourceTest, EventsFor_ReturnsEmptyForTickWithNothingRecorded)
{
    ReplaySource source({
        TimedEvent{.tick = 0, .event = Event{.name = "a"}},
    });

    EXPECT_TRUE(source.eventsFor(5).empty());
}
