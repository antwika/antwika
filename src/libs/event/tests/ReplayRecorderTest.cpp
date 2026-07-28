#include <gtest/gtest.h>

#include "antwika/event/ReplayRecorder.hpp"

using antwika::event::Event;
using antwika::event::ReplayRecorder;
using antwika::event::TimedEvent;

TEST(ReplayRecorderTest, handle)
{
    ReplayRecorder replayRecorder;
    replayRecorder.handle(TimedEvent{.tick = 0, .event = Event{.name = "foo"}});
    replayRecorder.handle(TimedEvent{.tick = 1, .event = Event{.name = "bar"}});
    auto events = replayRecorder.getEvents();
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(
        events[0], (TimedEvent{.tick = 0, .event = Event{.name = "foo"}}));
    EXPECT_EQ(
        events[1], (TimedEvent{.tick = 1, .event = Event{.name = "bar"}}));
}
