#include <gtest/gtest.h>

#include <vector>

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

TEST(ReplaySourceTest, EventsFor_WalksTheRecordingOnceAcrossASession)
{
    // What a loop does: every tick from zero, once each, in order.
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
        TickEvent{.tick = 2, .event = Event{.name = "b"}},
        TickEvent{.tick = 2, .event = Event{.name = "c"}},
        TickEvent{.tick = 3, .event = Event{.name = "d"}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = "a"}}));
    EXPECT_TRUE(source.eventsFor(1).empty());
    EXPECT_EQ(
        source.eventsFor(2),
        (std::vector<Event>{Event{.name = "b"}, Event{.name = "c"}}));
    EXPECT_EQ(source.eventsFor(3), (std::vector<Event>{Event{.name = "d"}}));
    EXPECT_TRUE(source.eventsFor(4).empty());
}

TEST(ReplaySourceTest, EventsFor_ReplaysAFileWhoseTicksAreOutOfOrder)
{
    // A hand-authored file need not be written in tick order.
    // The cursor can only walk forwards, so the constructor sorts.
    ReplaySource source({
        TickEvent{.tick = 2, .event = Event{.name = "b"}},
        TickEvent{.tick = 0, .event = Event{.name = "a"}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = "a"}}));
    EXPECT_EQ(source.eventsFor(2), (std::vector<Event>{Event{.name = "b"}}));
}

TEST(ReplaySourceTest, EventsFor_KeepsTheOrderOfEventsSharingATick)
{
    // Sorting is stable, because order within a tick is dispatch order.
    ReplaySource source({
        TickEvent{.tick = 1, .event = Event{.name = "b"}},
        TickEvent{.tick = 1, .event = Event{.name = "a"}},
        TickEvent{.tick = 0, .event = Event{.name = "c"}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = "c"}}));
    EXPECT_EQ(
        source.eventsFor(1),
        (std::vector<Event>{Event{.name = "b"}, Event{.name = "a"}}));
}

TEST(ReplaySourceTest, EventsFor_HandsAPayloadOutRatherThanCopyingIt)
{
    // A payload is copied out of the file once, not once per tick.
    ReplaySource source({TickEvent{
        .tick = 0,
        .event = Event{.name = "a", .payload = R"({"x":1})"}}});

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].payload, R"({"x":1})");
}
