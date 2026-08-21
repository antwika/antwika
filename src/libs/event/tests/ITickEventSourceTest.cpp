#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/event/mocks/MockTickEventSource.hpp>

#include "antwika/event/Event.hpp"
#include "antwika/event/ITickEventSource.hpp"

using antwika::event::Event;
using antwika::event::ITickEventSource;
using antwika::event::mocks::MockTickEventSource;

TEST(ITickEventSourceTest, EventsFor_CarriesTheTickToTheImplementation)
{
    MockTickEventSource source;
    ITickEventSource &asInterfaceSource = source;

    EXPECT_CALL(source, eventsFor(7))
        .WillOnce(
            ::testing::Return(std::vector<Event>{Event{.name = "tick-7"}}));

    const auto events = asInterfaceSource.eventsFor(7);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, "tick-7");
}
