#include <gtest/gtest.h>

#include "antwika/replay/ReplaySource.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;

TEST(ReplaySourceTest, EventsFor_ReturnsEventsRecordedAtThatTickInOrder)
{
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
        TickEvent{.tick = 1, .event = Event{.name = "b"}},
        TickEvent{.tick = 1, .event = Event{.name = "c"}},
    });

    auto tick1Events = source.eventsFor(1);

    ASSERT_EQ(tick1Events.size(), 2);
    EXPECT_EQ(tick1Events[0], (Event{.name = "b"}));
    EXPECT_EQ(tick1Events[1], (Event{.name = "c"}));
}

TEST(ReplaySourceTest, EventsFor_ReturnsEmptyForTickWithNothingRecorded)
{
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
    });

    EXPECT_TRUE(source.eventsFor(5).empty());
}

TEST(ReplaySourceTest, EventsFor_ReturnsEmptyWhenNoEventsWereEverRecorded)
{
    ReplaySource source({});

    EXPECT_TRUE(source.eventsFor(0).empty());
}
