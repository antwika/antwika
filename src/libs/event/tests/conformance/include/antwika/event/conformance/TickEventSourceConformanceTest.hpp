#pragma once

#include <gtest/gtest.h>

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::event::conformance
{

    inline constexpr antwika::time::Tick kRaiseTick = 5;

    inline constexpr antwika::time::Tick kFarTick = 1000000;

    [[nodiscard]] inline std::vector<Event> getRaisedEvents()
    {
        return {
            Event{.name = EventName{"conformance.first"}},
            Event{.name = EventName{"conformance.second"}}};
    }

    template <typename SourceTraits>
    class TickEventSourceConformanceTest : public ::testing::Test
    {
    protected:
        SourceTraits traits;
    };

    TYPED_TEST_SUITE_P(TickEventSourceConformanceTest);

    TYPED_TEST_P(
        TickEventSourceConformanceTest,
        EventsFor_GivesNothingWhenNothingHasHappened)
    {
        EXPECT_TRUE(this->traits.getSource().eventsFor(0).empty());
        EXPECT_TRUE(this->traits.getSource().eventsFor(1).empty());
    }

    TYPED_TEST_P(
        TickEventSourceConformanceTest,
        EventsFor_CarriesRaisedEventsThroughInOrder)
    {
        this->traits.raiseTwoEvents();

        EXPECT_EQ(
            this->traits.getSource().eventsFor(kRaiseTick),
            getRaisedEvents());
    }

    TYPED_TEST_P(
        TickEventSourceConformanceTest,
        EventsFor_HandsEachRaisedEventOutOnlyOnce)
    {
        this->traits.raiseTwoEvents();

        const auto first = this->traits.getSource().eventsFor(kRaiseTick);
        const auto second = this->traits.getSource().eventsFor(kRaiseTick);

        EXPECT_EQ(first.size(), 2U);
        EXPECT_TRUE(second.empty());
    }

    TYPED_TEST_P(
        TickEventSourceConformanceTest, EventsFor_ToleratesTicksNearAndFar)
    {
        EXPECT_NO_THROW({
            static_cast<void>(this->traits.getSource().eventsFor(0));
            static_cast<void>(this->traits.getSource().eventsFor(kFarTick));
        });
    }

    REGISTER_TYPED_TEST_SUITE_P(
        TickEventSourceConformanceTest,
        EventsFor_GivesNothingWhenNothingHasHappened,
        EventsFor_CarriesRaisedEventsThroughInOrder,
        EventsFor_HandsEachRaisedEventOutOnlyOnce,
        EventsFor_ToleratesTicksNearAndFar);

}
