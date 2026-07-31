#include "antwika/sound/WaveformId.hpp"

#include <gtest/gtest.h>

using antwika::sound::rawValue;
using antwika::sound::WaveformId;

// An empty scoped enum rather than an alias.
// An id cannot then have arithmetic done to it by accident.
TEST(WaveformIdTest, UnwrapsToTheNumberItHolds)
{
    EXPECT_EQ(rawValue(static_cast<WaveformId>(7)), 7U);
}

TEST(WaveformIdTest, TwoIdsCompareByTheirNumber)
{
    EXPECT_EQ(static_cast<WaveformId>(3), static_cast<WaveformId>(3));
    EXPECT_NE(static_cast<WaveformId>(3), static_cast<WaveformId>(4));
}
