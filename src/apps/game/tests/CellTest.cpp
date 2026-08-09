#include <gtest/gtest.h>

#include <compare>
#include <map>
#include <vector>

#include "antwika/game/Cell.hpp"

using antwika::game::Cell;

TEST(CellTest, Ctor_SitsAtTheOrigin)
{
    constexpr Cell cell;

    EXPECT_EQ(cell.x, 0);
    EXPECT_EQ(cell.y, 0);
}

TEST(CellTest, OperatorEquals_ComparesBothCoordinates)
{
    constexpr Cell cell{.x = 3, .y = -4};

    EXPECT_EQ(cell, (Cell{.x = 3, .y = -4}));
    EXPECT_NE(cell, (Cell{.x = 3, .y = 4}));
    EXPECT_NE(cell, (Cell{.x = -3, .y = -4}));
}

TEST(CellTest, OperatorCompare_OrdersByXAndThenY)
{
    EXPECT_LT((Cell{.x = 0, .y = 5}), (Cell{.x = 1, .y = 0}));
    EXPECT_LT((Cell{.x = 1, .y = 0}), (Cell{.x = 1, .y = 1}));
    EXPECT_GT((Cell{.x = 1, .y = 1}), (Cell{.x = 1, .y = 0}));
}

TEST(CellTest, OperatorCompare_PutsNegativesBelowPositives)
{
    EXPECT_LT((Cell{.x = -1, .y = 0}), (Cell{.x = 0, .y = 0}));
    EXPECT_LT((Cell{.x = 0, .y = -1}), (Cell{.x = 0, .y = 0}));
}

TEST(CellTest, OperatorCompare_KeysAMapInAStableOrder)
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

TEST(CellTest, OperatorCompare_AgreesWithTheOtherOperators)
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
