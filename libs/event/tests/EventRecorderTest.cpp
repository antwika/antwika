#include <gtest/gtest.h>

#include "antwika/event/EventRecorder.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;

TEST(EventRecorderTest, record)
{
    EventRecorder eventRecorder;
    eventRecorder.record(Event{.name = "foo"});
    eventRecorder.record(Event{.name = "bar"});
    auto events = eventRecorder.getEvents();
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].name, "foo");
    EXPECT_EQ(events[1].name, "bar");
}
