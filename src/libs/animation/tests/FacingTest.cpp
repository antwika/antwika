#include <gtest/gtest.h>

#include <antwika/animation/Facing.hpp>

namespace antwika::animation
{

    TEST(FacingTest, FacingIndex_IsContiguousFromZero)
    {
        EXPECT_EQ(facingIndex(Facing::North), 0U);
        EXPECT_EQ(facingIndex(Facing::East), 1U);
        EXPECT_EQ(facingIndex(Facing::South), 2U);
        EXPECT_EQ(facingIndex(Facing::West), 3U);
    }

    TEST(FacingTest, FacingCount_CountsEveryEnumerator)
    {
        EXPECT_EQ(kFacingCount, 4U);
    }

} // namespace antwika::animation
