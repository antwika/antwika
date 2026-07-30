#include <gtest/gtest.h>

#include "antwika/event/TickEventRecorder.hpp"

using antwika::event::Event;
using antwika::event::TickEventRecorder;
using antwika::event::TickEvent;

TEST(TickEventRecorderTest, handle)
{
    TickEventRecorder replayRecorder;
    replayRecorder.handle(TickEvent{.tick = 0, .event = Event{.name = "foo"}});
    replayRecorder.handle(TickEvent{.tick = 1, .event = Event{.name = "bar"}});
    auto events = replayRecorder.getEvents();
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(
        events[0], (TickEvent{.tick = 0, .event = Event{.name = "foo"}}));
    EXPECT_EQ(
        events[1], (TickEvent{.tick = 1, .event = Event{.name = "bar"}}));
}

TEST(TickEventRecorderTest, GetEvents_HandsBackTheRecordingItself)
{
    // By reference, so asking what has been recorded costs nothing.
    TickEventRecorder replayRecorder;
    const auto &events = replayRecorder.getEvents();

    EXPECT_TRUE(events.empty());

    replayRecorder.handle(TickEvent{.tick = 0, .event = Event{.name = "foo"}});

    EXPECT_EQ(events.size(), 1U);
}
