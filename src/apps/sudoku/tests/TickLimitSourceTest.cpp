#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include <antwika/sudoku/TickLimitSource.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;
using antwika::sudoku::TickLimitSource;

namespace
{
    [[nodiscard]] std::vector<TickEvent> scripted()
    {
        return {TickEvent{
            .tick = 1, .event = Event{.name = "sudoku.solve"}}};
    }

    TEST(TickLimitSourceTest, EventsFor_AsksToStopOnceTheCapIsReached)
    {
        ReplaySource inner(scripted());
        TickLimitSource source(inner, 1);

        EXPECT_TRUE(source.eventsFor(0).empty());

        const auto capped = source.eventsFor(1);

        ASSERT_EQ(capped.size(), 2U);
        EXPECT_EQ(capped[0].name, "sudoku.solve");
        EXPECT_EQ(capped[1].name, antwika::engine::events::kStop);
    }

    TEST(TickLimitSourceTest, EventsFor_NeverStopsWithoutACap)
    {
        ReplaySource inner(scripted());
        TickLimitSource source(inner, std::nullopt);

        EXPECT_TRUE(source.eventsFor(0).empty());
        EXPECT_EQ(source.eventsFor(1).size(), 1U);
    }
} // namespace
