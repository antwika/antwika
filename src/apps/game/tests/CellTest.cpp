#include <gtest/gtest.h>

#include <compare>
#include <map>
#include <vector>

#include "antwika/game/Cell.hpp"

using antwika::game::Cell;

TEST(CellTest, DefaultConstructedSitsAtTheOrigin)
{
    constexpr Cell cell;

    EXPECT_EQ(cell.x, 0);
    EXPECT_EQ(cell.y, 0);
}

TEST(CellTest, EqualityComparesBothCoordinatesIndependently)
{
    constexpr Cell cell{.x = 3, .y = -4};

    EXPECT_EQ(cell, (Cell{.x = 3, .y = -4}));
    EXPECT_NE(cell, (Cell{.x = 3, .y = 4}));
    EXPECT_NE(cell, (Cell{.x = -3, .y = -4}));
}

TEST(CellTest, OrdersByXAndThenY)
{
    EXPECT_LT((Cell{.x = 0, .y = 5}), (Cell{.x = 1, .y = 0}));
    EXPECT_LT((Cell{.x = 1, .y = 0}), (Cell{.x = 1, .y = 1}));
    EXPECT_GT((Cell{.x = 1, .y = 1}), (Cell{.x = 1, .y = 0}));
}

TEST(CellTest, OrdersNegativeCoordinatesBelowPositiveOnes)
{
    EXPECT_LT((Cell{.x = -1, .y = 0}), (Cell{.x = 0, .y = 0}));
    EXPECT_LT((Cell{.x = 0, .y = -1}), (Cell{.x = 0, .y = 0}));
}

// The order decides what gets drawn first.
// So it has to be the same every run, not merely exist.
TEST(CellTest, KeysAMapInAStableOrder)
{
    const std::map<Cell, int> cells{
        {{.x = 1, .y = 1}, 3},
        {{.x = 0, .y = 2}, 1},
        {{.x = 1, .y = 0}, 2},
        {{.x = -1, .y = 9}, 0},
    };

    int expected = 0;
    for (const auto &[cell, order] : cells)
    {
        EXPECT_EQ(order, expected++);
    }
}

// Exercises the defaulted three-way comparison in every direction.
// The values come from a vector, so nothing folds away.
TEST(CellTest, ThreeWayComparisonAgreesWithTheOtherOperators)
{
    const std::vector<Cell> cells{
        {.x = 0, .y = 0},
        {.x = 0, .y = 1},
        {.x = 1, .y = 0},
        {.x = 1, .y = 1},
        {.x = -1, .y = 0},
    };

    for (const auto &left : cells)
    {
        for (const auto &right : cells)
        {
            const auto ordering = left <=> right;

            EXPECT_EQ(
                ordering == std::strong_ordering::equal, left == right);
            EXPECT_EQ(ordering == std::strong_ordering::less, left < right);
            EXPECT_EQ(
                ordering == std::strong_ordering::greater, left > right);
            EXPECT_EQ(ordering != std::strong_ordering::greater,
                      left <= right);
            EXPECT_EQ(ordering != std::strong_ordering::less,
                      left >= right);
        }
    }
}
