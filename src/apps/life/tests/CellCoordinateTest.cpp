#include <gtest/gtest.h>

#include "antwika/life/CellCoordinate.hpp"

using antwika::life::CellCoordinate;

TEST(CellCoordinateTest, OperatorEquals_ComparesBothIndexes)
{
    constexpr CellCoordinate cell{.x = 2, .y = 3};

    EXPECT_EQ(cell, (CellCoordinate{.x = 2, .y = 3}));
    EXPECT_NE(cell, (CellCoordinate{.x = 9, .y = 3}));
    EXPECT_NE(cell, (CellCoordinate{.x = 2, .y = 9}));
}
