#include "antwika/music_editor/TickBudgetSource.hpp"

#include <vector>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

using antwika::event::Event;
using antwika::music_editor::TickBudgetSource;
using antwika::simulation::ITickEventSource;

namespace
{
    // Says one thing of its own, so a test can see what was forwarded.
    class OneEventSource final : public ITickEventSource
    {
    public:
        std::vector<Event> eventsFor(antwika::time::Tick) override
        {
            return {Event{.name = "test.something"}};
        }
    };

    [[nodiscard]] bool holdsStop(const std::vector<Event> &events)
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
} // namespace

TEST(TickBudgetSourceTest, PassesTheInnerSourcesEventsThrough)
{
    OneEventSource inner;
    TickBudgetSource source(inner, 4);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, "test.something");
}

TEST(TickBudgetSourceTest, SaysNothingUntilTheBudgetIsReached)
{
    OneEventSource inner;
    TickBudgetSource source(inner, 4);

    EXPECT_FALSE(holdsStop(source.eventsFor(3)));
}

// The tick with the budget's number is the one carrying the stop.
TEST(TickBudgetSourceTest, StopsOnTheTickItsBudgetNames)
{
    OneEventSource inner;
    TickBudgetSource source(inner, 4);

    EXPECT_TRUE(holdsStop(source.eventsFor(4)));
    EXPECT_TRUE(holdsStop(source.eventsFor(5)));
}

TEST(TickBudgetSourceTest, ABudgetOfNothingStopsOnTheFirstTick)
{
    OneEventSource inner;
    TickBudgetSource source(inner, 0);

    EXPECT_TRUE(holdsStop(source.eventsFor(0)));
}
