#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/geometry/Point.hpp>
#include <antwika/geometry/Rect.hpp>
#include <antwika/geometry/Size.hpp>

using antwika::geometry::cellAt;
using antwika::geometry::cellRect;
using antwika::geometry::Grid;
using antwika::geometry::GridCell;
using antwika::geometry::gridFit;
using antwika::geometry::Point;
using antwika::geometry::Rect;
using antwika::geometry::Size;

namespace
{
    Rect areaOf(const std::uint32_t width, const std::uint32_t height)
    {
        return Rect{
            .origin = Point{},
            .size = Size{.width = width, .height = height}};
    }
} // namespace

TEST(GridTest, FitsTheLargestWholePixelCellAndCentresIt)
{
    const auto grid = gridFit(areaOf(100, 100), 8, 8);

    ASSERT_TRUE(grid.has_value());

    // 100 / 8 is 12 with 4 pixels left over, 2 to a side.
    EXPECT_EQ(grid->cell, 12U);
    EXPECT_EQ(grid->origin, (Point{.x = 2, .y = 2}));
    EXPECT_EQ(grid->columns, 8U);
    EXPECT_EQ(grid->rows, 8U);
}

// The narrow dimension is what decides the cell, either way round.
TEST(GridTest, TheSmallerOfTheTwoFitsDecidesTheCell)
{
    const auto wide = gridFit(areaOf(100, 40), 4, 4);
    const auto tall = gridFit(areaOf(40, 100), 4, 4);

    ASSERT_TRUE(wide.has_value());
    ASSERT_TRUE(tall.has_value());
    EXPECT_EQ(wide->cell, 10U);
    EXPECT_EQ(tall->cell, 10U);

    // Centred in the dimension the other one was decided by.
    EXPECT_EQ(wide->origin, (Point{.x = 30, .y = 0}));
    EXPECT_EQ(tall->origin, (Point{.x = 0, .y = 30}));
}

// The area need not start at the canvas's corner.
TEST(GridTest, CentresInsideTheAreaItWasGiven)
{
    const auto grid = gridFit(
        Rect{
            .origin = Point{.x = 30, .y = -7},
            .size = Size{.width = 100, .height = 100}},
        8,
        8);

    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->origin, (Point{.x = 32, .y = -5}));
}

TEST(GridTest, AGridWithNoColumnsOrNoRowsFitsNothing)
{
    EXPECT_FALSE(gridFit(areaOf(100, 100), 0, 8).has_value());
    EXPECT_FALSE(gridFit(areaOf(100, 100), 8, 0).has_value());
}

TEST(GridTest, AnAreaTooSmallForAPixelEachFitsNothing)
{
    EXPECT_FALSE(gridFit(areaOf(7, 100), 8, 8).has_value());
    EXPECT_FALSE(gridFit(areaOf(100, 7), 8, 8).has_value());
}

TEST(GridTest, ReadsAPointInsideTheGridAsACell)
{
    const Grid grid{
        .origin = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_EQ(
        cellAt(grid, Point{.x = 10, .y = 20}),
        (GridCell{.column = 0, .row = 0}));
    EXPECT_EQ(
        cellAt(grid, Point{.x = 24, .y = 26}),
        (GridCell{.column = 2, .row = 1}));
    EXPECT_EQ(
        cellAt(grid, Point{.x = 29, .y = 34}),
        (GridCell{.column = 3, .row = 2}));
}

TEST(GridTest, APointBeforeTheOriginIsInNoCell)
{
    const Grid grid{
        .origin = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(cellAt(grid, Point{.x = 9, .y = 20}).has_value());
    EXPECT_FALSE(cellAt(grid, Point{.x = 10, .y = 19}).has_value());
}

TEST(GridTest, APointPastTheLastCellIsInNoCell)
{
    const Grid grid{
        .origin = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(cellAt(grid, Point{.x = 30, .y = 20}).has_value());
    EXPECT_FALSE(cellAt(grid, Point{.x = 10, .y = 35}).has_value());
}

// Widened before subtracting, so a pointer this far out misses.
// Subtracted in 32 bits it would wrap and land back inside.
TEST(GridTest, APointFarOutsideMissesRatherThanWraps)
{
    const Grid grid{
        .origin = Point{.x = -2147483647, .y = -2147483647},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(
        cellAt(grid, Point{.x = 2147483647, .y = 2147483647})
            .has_value());
}

// A grid nothing fitted into divides by nothing, so it is asked first.
TEST(GridTest, AGridOfNoCellsHoldsNoPointAtAll)
{
    Grid grid;
    grid.columns = 4;
    grid.rows = 3;

    EXPECT_FALSE(cellAt(grid, Point{}).has_value());
}

TEST(GridTest, PlacesACellBackWhereItCameFrom)
{
    const Grid grid{
        .origin = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_EQ(
        cellRect(grid, GridCell{.column = 2, .row = 1}),
        (Rect{
            .origin = Point{.x = 20, .y = 25},
            .size = Size{.width = 5, .height = 5}}));
}
