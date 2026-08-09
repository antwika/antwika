#include <gtest/gtest.h>

#include "antwika/sound/WaveformId.hpp"

using antwika::sound::rawValue;
using antwika::sound::WaveformId;

TEST(WaveformIdTest, RawValue_UnwrapsToTheNumberItHolds)
{
    EXPECT_EQ(rawValue(static_cast<WaveformId>(7)), 7U);
}

TEST(WaveformIdTest, OperatorCompare_ComparesIdsByTheirNumber)
{
    EXPECT_EQ(static_cast<WaveformId>(3), static_cast<WaveformId>(3));
    EXPECT_NE(static_cast<WaveformId>(3), static_cast<WaveformId>(4));
}
