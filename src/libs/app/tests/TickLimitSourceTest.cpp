#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/app/TickLimitSource.hpp"

using antwika::app::TickLimitSource;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;

namespace
{
    std::vector<TickEvent> getOneEventOnTickTwo()
    {
        return {TickEvent{
            .tick = 2, .event = Event{.name = "app.something"}}};
    }

    bool holdsStop(const std::vector<Event> &events)
    {
        for (const Event &event : events)
        {
            if (event.name == antwika::engine::events::kStop)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(TickLimitSourceTest, EventsFor_PassesEveryTickBeforeTheCapThrough)
{
    ReplaySource innerSource(getOneEventOnTickTwo());
    TickLimitSource source(innerSource, 3);

    const auto events = source.eventsFor(2);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.front().name, "app.something");
}

TEST(TickLimitSourceTest, EventsFor_AsksToStopOnceTheCapIsReached)
{
    ReplaySource innerSource(getOneEventOnTickTwo());
    TickLimitSource source(innerSource, 2);

    const auto events = source.eventsFor(2);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events.front().name, "app.something");
    EXPECT_EQ(events.back().name, antwika::engine::events::kStop);
    EXPECT_TRUE(holdsStop(source.eventsFor(3)));
}

TEST(TickLimitSourceTest, EventsFor_NeverStopsWhenThereIsNoCap)
{
    ReplaySource innerSource(getOneEventOnTickTwo());
    TickLimitSource source(innerSource, std::nullopt);

    EXPECT_FALSE(holdsStop(source.eventsFor(2)));
    EXPECT_FALSE(holdsStop(source.eventsFor(90000)));
}
