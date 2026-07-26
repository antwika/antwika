#include <gtest/gtest.h>

#include "antwika/event/Event.hpp"

using antwika::event::Event;

TEST(EventTest, DefaultConstruction)
{
    Event event;
    EXPECT_EQ(event.name, "");
}

TEST(EventTest, AggregateConstruction)
{
    Event event{.name = "Event"};
    EXPECT_EQ(event.name, "Event");
}

TEST(EventTest, Equality)
{
    Event event1{.name = "Event"};
    Event event2{.name = "Event"};
    EXPECT_EQ(event1, event2);
}

TEST(EventTest, Inequality)
{
    Event event1{.name = "Event 1"};
    Event event2{.name = "Event 2"};
    EXPECT_NE(event1, event2);
}
