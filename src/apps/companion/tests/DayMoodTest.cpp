#include <cstdint>
#include <set>

#include <gtest/gtest.h>

#include "antwika/companion/DayMood.hpp"

using antwika::companion::DayMood;
using antwika::companion::moodOn;

namespace
{
    // The first day of a companion's life is deliberately plain.
    // Every trace in the simulation's own tests runs on plain periods.
    // A hurried first day would move every one of them.
    TEST(DayMoodTest, TheFirstDayIsOrdinary)
    {
        EXPECT_EQ(moodOn(0), DayMood::Ordinary);
    }

    TEST(DayMoodTest, TheSameDayIsAlwaysTheSameKindOfDay)
    {
        for (std::uint32_t day = 0; day < 50; ++day)
        {
            EXPECT_EQ(moodOn(day), moodOn(day));
        }
    }

    // A hash rather than a rota, so the four do not come round in turn.
    // And every one of them does come round.
    TEST(DayMoodTest, EveryMoodTurnsUpAcrossEnoughDays)
    {
        std::set<DayMood> seen;

        for (std::uint32_t day = 0; day < 200; ++day)
        {
            seen.insert(moodOn(day));
        }

        EXPECT_EQ(seen.size(), 4U);
    }

    // Half the slots are ordinary.
    // So a mood reads as a break from the ordinary rather than the rule.
    TEST(DayMoodTest, AboutHalfOfAllDaysAreOrdinary)
    {
        std::uint32_t ordinary = 0;

        for (std::uint32_t day = 0; day < 600; ++day)
        {
            if (moodOn(day) == DayMood::Ordinary)
            {
                ++ordinary;
            }
        }

        EXPECT_GT(ordinary, 200U);
        EXPECT_LT(ordinary, 400U);
    }
} // namespace
