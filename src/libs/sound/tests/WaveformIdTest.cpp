#include <gtest/gtest.h>

#include "antwika/sound/WaveformId.hpp"

using antwika::sound::getRawValue;
using antwika::sound::WaveformId;

TEST(WaveformIdTest, RawValue_UnwrapsToTheNumberItHolds)
{
    EXPECT_EQ(getRawValue(static_cast<WaveformId>(7)), 7U);
}

TEST(WaveformIdTest, OperatorCompare_ComparesIdsByTheirNumber)
{
    EXPECT_EQ(static_cast<WaveformId>(3), static_cast<WaveformId>(3));
    EXPECT_NE(static_cast<WaveformId>(3), static_cast<WaveformId>(4));
}
