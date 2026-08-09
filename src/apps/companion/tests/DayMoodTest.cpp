#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include "antwika/companion/DayMood.hpp"

using antwika::companion::DayMood;
using antwika::companion::moodOn;

namespace
{
    TEST(DayMoodTest, MoodOn_MakesTheFirstDayOrdinary)
    {
        EXPECT_EQ(moodOn(0), DayMood::Ordinary);
    }

    TEST(DayMoodTest, MoodOn_MatchesTheRecordedFirstFortnight)
    {
        const std::vector<DayMood> expected{
            DayMood::Ordinary, DayMood::Ordinary, DayMood::Heavy,
            DayMood::Ordinary, DayMood::Restless, DayMood::Ordinary,
            DayMood::Restless, DayMood::Ordinary, DayMood::Hungry,
            DayMood::Ordinary, DayMood::Hungry, DayMood::Ordinary};

        std::vector<DayMood> moods;
        for (std::uint32_t day = 0; day < expected.size(); ++day)
        {
            moods.push_back(moodOn(day));
        }

        EXPECT_EQ(moods, expected);
    }

    TEST(DayMoodTest, MoodOn_ReachesEveryMoodEventually)
    {
        std::set<DayMood> seen;

        for (std::uint32_t day = 0; day < 200; ++day)
        {
            seen.insert(moodOn(day));
        }

        EXPECT_EQ(seen.size(), 4U);
    }

    TEST(DayMoodTest, MoodOn_MakesAboutHalfOfDaysOrdinary)
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
}
