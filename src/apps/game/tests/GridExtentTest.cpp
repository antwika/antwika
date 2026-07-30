#include <gtest/gtest.h>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

using antwika::game::Cell;
using antwika::game::GridExtent;

TEST(GridExtentTest, Contains_AcceptsACellInsideTheBounds)
{
    constexpr GridExtent extent{.width = 8, .height = 4};

    EXPECT_TRUE(extent.contains(Cell{.x = 0, .y = 0}));
    EXPECT_TRUE(extent.contains(Cell{.x = 7, .y = 3}));
    EXPECT_TRUE(extent.contains(Cell{.x = 4, .y = 2}));
}

TEST(GridExtentTest, Contains_RejectsEachEdgeIndependently)
{
    constexpr GridExtent extent{.width = 8, .height = 4};

    EXPECT_FALSE(extent.contains(Cell{.x = -1, .y = 0}));
    EXPECT_FALSE(extent.contains(Cell{.x = 8, .y = 0}));
    EXPECT_FALSE(extent.contains(Cell{.x = 0, .y = -1}));
    EXPECT_FALSE(extent.contains(Cell{.x = 0, .y = 4}));
}

TEST(GridExtentTest, Contains_RejectsEverythingWhenEmpty)
{
    constexpr GridExtent empty{};

    EXPECT_FALSE(empty.contains(Cell{.x = 0, .y = 0}));
}

TEST(GridExtentTest, EqualityComparesBothDimensions)
{
    constexpr GridExtent extent{.width = 8, .height = 4};

    EXPECT_EQ(extent, (GridExtent{.width = 8, .height = 4}));
    EXPECT_NE(extent, (GridExtent{.width = 8, .height = 5}));
    EXPECT_NE(extent, (GridExtent{.width = 4, .height = 4}));
}
