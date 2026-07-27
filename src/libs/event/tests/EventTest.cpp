#include <gtest/gtest.h>

#include "antwika/event/Event.hpp"

using antwika::event::Event;

TEST(EventTest, DefaultConstruction)
{
    Event event;
    EXPECT_EQ(event.name, "");
    EXPECT_EQ(event.payload, "");
}

TEST(EventTest, AggregateConstruction)
{
    Event event{.name = "Event"};
    EXPECT_EQ(event.name, "Event");
    EXPECT_EQ(event.payload, "");
}

TEST(EventTest, AggregateConstructionWithPayload)
{
    Event event{.name = "Event", .payload = "payload-bytes"};
    EXPECT_EQ(event.name, "Event");
    EXPECT_EQ(event.payload, "payload-bytes");
}

TEST(EventTest, Equality)
{
    Event event1{.name = "Event", .payload = "payload"};
    Event event2{.name = "Event", .payload = "payload"};
    EXPECT_EQ(event1, event2);
}

TEST(EventTest, Inequality)
{
    Event event1{.name = "Event 1"};
    Event event2{.name = "Event 2"};
    EXPECT_NE(event1, event2);
}

TEST(EventTest, InequalityWhenOnlyPayloadDiffers)
{
    Event event1{.name = "Event", .payload = "a"};
    Event event2{.name = "Event", .payload = "b"};
    EXPECT_NE(event1, event2);
}
