#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/geometry/Point.hpp>
#include <antwika/geometry/Rect.hpp>
#include <antwika/geometry/Size.hpp>

using antwika::geometry::cellAt;
using antwika::geometry::getCellRect;
using antwika::geometry::Grid;
using antwika::geometry::GridCell;
using antwika::geometry::getGridFit;
using antwika::geometry::getGridFitBelow;
using antwika::geometry::Point;
using antwika::geometry::Rect;
using antwika::geometry::Size;

namespace
{
    Rect areaOf(const std::uint32_t width, const std::uint32_t height)
    {
        return Rect{
            .originPoint = Point{},
            .size = Size{.width = width, .height = height}};
    }
}

TEST(GridTest, GridFit_FitsTheLargestWholePixelCellAndCentresIt)
{
    const auto grid = getGridFit(areaOf(100, 100), 8, 8);

    ASSERT_TRUE(grid.has_value());

    EXPECT_EQ(grid->cell, 12U);
    EXPECT_EQ(grid->originPoint, (Point{.x = 2, .y = 2}));
    EXPECT_EQ(grid->columns, 8U);
    EXPECT_EQ(grid->rows, 8U);
}

TEST(GridTest, GridFit_TakesTheSmallerOfTheTwoFits)
{
    const auto wideGrid = getGridFit(areaOf(100, 40), 4, 4);
    const auto tallGrid = getGridFit(areaOf(40, 100), 4, 4);

    ASSERT_TRUE(wideGrid.has_value());
    ASSERT_TRUE(tallGrid.has_value());
    EXPECT_EQ(wideGrid->cell, 10U);
    EXPECT_EQ(tallGrid->cell, 10U);

    EXPECT_EQ(wideGrid->originPoint, (Point{.x = 30, .y = 0}));
    EXPECT_EQ(tallGrid->originPoint, (Point{.x = 0, .y = 30}));
}

TEST(GridTest, GridFit_CentresInsideTheAreaItWasGiven)
{
    const auto grid = getGridFit(
        Rect{
            .originPoint = Point{.x = 30, .y = -7},
            .size = Size{.width = 100, .height = 100}},
        8,
        8);

    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->originPoint, (Point{.x = 32, .y = -5}));
}

TEST(GridTest, GridFit_FitsNothingWithNoColumnsOrRows)
{
    EXPECT_FALSE(getGridFit(areaOf(100, 100), 0, 8).has_value());
    EXPECT_FALSE(getGridFit(areaOf(100, 100), 8, 0).has_value());
}

TEST(GridTest, GridFit_FitsNothingInATooSmallArea)
{
    EXPECT_FALSE(getGridFit(areaOf(7, 100), 8, 8).has_value());
    EXPECT_FALSE(getGridFit(areaOf(100, 7), 8, 8).has_value());
}

TEST(GridTest, CellAt_ReadsAPointInsideTheGridAsACell)
{
    const Grid grid{
        .originPoint = Point{.x = 10, .y = 20},
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

TEST(GridTest, OperatorEquals_SeparatesCellsDifferingInEitherIndex)
{
    constexpr GridCell cell{.column = 2, .row = 1};

    EXPECT_NE(cell, (GridCell{.column = 3, .row = 1}));
    EXPECT_NE(cell, (GridCell{.column = 2, .row = 2}));
    EXPECT_EQ(cell, (GridCell{.column = 2, .row = 1}));
}

TEST(GridTest, GridCell_DefaultsToTheFirstCell)
{
    constexpr GridCell cell{};

    EXPECT_EQ(cell.column, 0U);
    EXPECT_EQ(cell.row, 0U);
}

TEST(GridTest, CellAt_FindsNoCellBeforeTheOrigin)
{
    const Grid grid{
        .originPoint = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(cellAt(grid, Point{.x = 9, .y = 20}).has_value());
    EXPECT_FALSE(cellAt(grid, Point{.x = 10, .y = 19}).has_value());
}

TEST(GridTest, CellAt_FindsNoCellPastTheLastCell)
{
    const Grid grid{
        .originPoint = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(cellAt(grid, Point{.x = 30, .y = 20}).has_value());
    EXPECT_FALSE(cellAt(grid, Point{.x = 10, .y = 35}).has_value());
}

TEST(GridTest, CellAt_MissesRatherThanWrapsFarOutside)
{
    const Grid grid{
        .originPoint = Point{.x = -2147483647, .y = -2147483647},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_FALSE(
        cellAt(grid, Point{.x = 2147483647, .y = 2147483647})
            .has_value());
}

TEST(GridTest, CellAt_FindsNoCellInAGridOfNone)
{
    Grid grid;
    grid.columns = 4;
    grid.rows = 3;

    EXPECT_FALSE(cellAt(grid, Point{}).has_value());
}

TEST(GridTest, CellRect_PlacesACellBackWhereItCameFrom)
{
    const Grid grid{
        .originPoint = Point{.x = 10, .y = 20},
        .cell = 5,
        .columns = 4,
        .rows = 3};

    EXPECT_EQ(
        getCellRect(grid, GridCell{.column = 2, .row = 1}),
        (Rect{
            .originPoint = Point{.x = 20, .y = 25},
            .size = Size{.width = 5, .height = 5}}));
}

TEST(GridTest, GridFitBelow_LeavesTheReservedBandAlone)
{
    const auto grid =
        getGridFitBelow(Size{.width = 100, .height = 120}, 20, 8, 8);

    ASSERT_TRUE(grid.has_value());

    EXPECT_EQ(grid->cell, 12U);
    EXPECT_EQ(grid->originPoint, (Point{.x = 2, .y = 22}));
}

TEST(GridTest, GridFitBelow_FitsNothingWhenTheBandTakesTheCanvas)
{
    EXPECT_FALSE(
        getGridFitBelow(Size{.width = 100, .height = 20}, 20, 8, 8)
            .has_value());
}

TEST(GridTest, GridFitBelow_FitsNothingWhenWhatIsLeftIsTooShort)
{
    EXPECT_FALSE(
        getGridFitBelow(Size{.width = 100, .height = 24}, 20, 8, 8)
            .has_value());
}

TEST(GridTest, OperatorEquals_ComparesEveryGridField)
{
    constexpr Grid baseGrid{
        .originPoint = {.x = 4, .y = 64}, .cell = 80, .columns = 12, .rows = 8};

    EXPECT_EQ(baseGrid, Grid{baseGrid});

    Grid otherGrid = baseGrid;
    otherGrid.columns = 11;
    EXPECT_NE(baseGrid, otherGrid);

    otherGrid = baseGrid;
    otherGrid.rows = 7;
    EXPECT_NE(baseGrid, otherGrid);

    otherGrid = baseGrid;
    otherGrid.cell = 79;
    EXPECT_NE(baseGrid, otherGrid);

    otherGrid = baseGrid;
    otherGrid.originPoint = {.x = 5, .y = 64};
    EXPECT_NE(baseGrid, otherGrid);
}
