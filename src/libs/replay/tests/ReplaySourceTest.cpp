#include <gtest/gtest.h>

#include <vector>

#include "antwika/replay/ReplaySource.hpp"

using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;

TEST(ReplaySourceTest, EventsFor_ReturnsEventsRecordedAtThatTickInOrder)
{
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = EventName{"a"}}},
        TickEvent{.tick = 1, .event = Event{.name = EventName{"b"}}},
        TickEvent{.tick = 1, .event = Event{.name = EventName{"c"}}},
    });

    auto tick1Events = source.eventsFor(1);

    ASSERT_EQ(tick1Events.size(), 2);
    EXPECT_EQ(tick1Events[0], (Event{.name = EventName{"b"}}));
    EXPECT_EQ(tick1Events[1], (Event{.name = EventName{"c"}}));
}

TEST(ReplaySourceTest, EventsFor_ReturnsEmptyForTickWithNothingRecorded)
{
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = EventName{"a"}}},
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
    ReplaySource source({
        TickEvent{.tick = 0, .event = Event{.name = EventName{"a"}}},
        TickEvent{.tick = 2, .event = Event{.name = EventName{"b"}}},
        TickEvent{.tick = 2, .event = Event{.name = EventName{"c"}}},
        TickEvent{.tick = 3, .event = Event{.name = EventName{"d"}}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = EventName{"a"}}}));
    EXPECT_TRUE(source.eventsFor(1).empty());
    EXPECT_EQ(
        source.eventsFor(2),
        (std::vector<Event>{Event{.name = EventName{"b"}}, Event{.name = EventName{"c"}}}));
    EXPECT_EQ(source.eventsFor(3), (std::vector<Event>{Event{.name = EventName{"d"}}}));
    EXPECT_TRUE(source.eventsFor(4).empty());
}

TEST(ReplaySourceTest, EventsFor_ReplaysAScriptedVectorThatIsOutOfOrder)
{
    ReplaySource source({
        TickEvent{.tick = 2, .event = Event{.name = EventName{"b"}}},
        TickEvent{.tick = 0, .event = Event{.name = EventName{"a"}}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = EventName{"a"}}}));
    EXPECT_EQ(source.eventsFor(2), (std::vector<Event>{Event{.name = EventName{"b"}}}));
}

TEST(ReplaySourceTest, EventsFor_KeepsTheOrderOfEventsSharingATick)
{
    ReplaySource source({
        TickEvent{.tick = 1, .event = Event{.name = EventName{"b"}}},
        TickEvent{.tick = 1, .event = Event{.name = EventName{"a"}}},
        TickEvent{.tick = 0, .event = Event{.name = EventName{"c"}}},
    });

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{Event{.name = EventName{"c"}}}));
    EXPECT_EQ(
        source.eventsFor(1),
        (std::vector<Event>{Event{.name = EventName{"b"}}, Event{.name = EventName{"a"}}}));
}

TEST(ReplaySourceTest, EventsFor_CarriesThePayloadAlongWithTheName)
{
    ReplaySource source({TickEvent{
        .tick = 0,
        .event = Event{.name = EventName{"a"}, .payload = R"({"x":1})"}}});

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, EventName{"a"});
    EXPECT_EQ(events[0].payload, R"({"x":1})");
}
