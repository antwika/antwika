#include <vector>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/ui_demo/TickBudgetSource.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;
using antwika::ui_demo::TickBudgetSource;

namespace
{
    [[nodiscard]] bool stops(const std::vector<Event> &events)
    {
        for (const auto &event : events)
        {
            if (event.name == antwika::engine::events::kStop)
            {
                return true;
            }
        }

        return false;
    }

    TEST(TickBudgetSourceTest, EventsFor_PassesATickInsideTheBudgetOn)
    {
        ReplaySource inner(
            {TickEvent{.tick = 1, .event = Event{.name = "a.thing"}}});
        TickBudgetSource source(inner, 3);

        const auto events = source.eventsFor(1);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].name, "a.thing");
    }

    TEST(TickBudgetSourceTest, EventsFor_StopsOnceTheBudgetIsUp)
    {
        ReplaySource inner({});
        TickBudgetSource source(inner, 3);

        EXPECT_FALSE(stops(source.eventsFor(2)));
        EXPECT_TRUE(stops(source.eventsFor(3)));
        EXPECT_TRUE(stops(source.eventsFor(4)));
    }
} // namespace
