#include <gtest/gtest.h>

#include "antwika/event/Event.hpp"

using antwika::event::Event;

TEST(EventTest, Ctor_DefaultsToEmptyFields)
{
    Event event;
    EXPECT_EQ(event.name, "");
    EXPECT_EQ(event.payload, "");
}

TEST(EventTest, Ctor_TakesANameAlone)
{
    Event event{.name = "Event"};
    EXPECT_EQ(event.name, "Event");
    EXPECT_EQ(event.payload, "");
}

TEST(EventTest, Ctor_TakesANameAndPayload)
{
    Event event{.name = "Event", .payload = "payload-bytes"};
    EXPECT_EQ(event.name, "Event");
    EXPECT_EQ(event.payload, "payload-bytes");
}

TEST(EventTest, OperatorEquals_MatchesOnEveryField)
{
    Event event1{.name = "Event", .payload = "payload"};
    Event event2{.name = "Event", .payload = "payload"};
    EXPECT_EQ(event1, event2);
}

TEST(EventTest, OperatorEquals_SeparatesDifferentNames)
{
    Event event1{.name = "Event 1"};
    Event event2{.name = "Event 2"};
    EXPECT_NE(event1, event2);
}

TEST(EventTest, OperatorEquals_SeparatesDifferentPayloads)
{
    Event event1{.name = "Event", .payload = "a"};
    Event event2{.name = "Event", .payload = "b"};
    EXPECT_NE(event1, event2);
}
