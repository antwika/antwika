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
