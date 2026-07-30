#include "antwika/life/BoardLayout.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::life::BoardLayout;
using antwika::life::CellCoordinate;
using antwika::life::cellAt;
using antwika::life::layoutFor;

namespace
{
    // Four cells of ten pixels each, filling the canvas exactly.
    constexpr Size kExactCanvas{.width = 40, .height = 40};

    [[nodiscard]] BoardLayout exactLayout()
    {
        return BoardLayout{
            .width = 4,
            .height = 4,
            .cell = 10,
            .origin = {.x = 0, .y = 0}};
    }
} // namespace

TEST(BoardLayoutTest, LayoutFor_FillsACanvasThatDividesExactly)
{
    EXPECT_EQ(layoutFor(kExactCanvas, 4, 4), exactLayout());
}

TEST(BoardLayoutTest, LayoutFor_CentresWhatIsLeftOver)
{
    // 45 / 4 rounds down to 11 pixels a cell.
    // That leaves one pixel each side of the 44 the board uses.
    const auto layout = layoutFor(Size{.width = 45, .height = 45}, 4, 4);

    ASSERT_TRUE(layout.has_value());
    EXPECT_EQ(layout->cell, 11u);
    EXPECT_EQ(layout->origin, (Point{.x = 0, .y = 0}));
}

TEST(BoardLayoutTest, LayoutFor_KeepsCellsSquareOnAnOblongCanvas)
{
    const auto layout = layoutFor(Size{.width = 100, .height = 40}, 4, 4);

    ASSERT_TRUE(layout.has_value());
    EXPECT_EQ(layout->cell, 10u);
    EXPECT_EQ(layout->origin, (Point{.x = 30, .y = 0}));
}

TEST(BoardLayoutTest, LayoutFor_ReturnsNulloptForABoardWithNoColumns)
{
    EXPECT_FALSE(layoutFor(kExactCanvas, 0, 4).has_value());
}

TEST(BoardLayoutTest, LayoutFor_ReturnsNulloptForABoardWithNoRows)
{
    EXPECT_FALSE(layoutFor(kExactCanvas, 4, 0).has_value());
}

// Cells are never rounded up to a minimum size.
// A canvas with fewer pixels than cells has nothing to draw at all.
TEST(BoardLayoutTest, LayoutFor_ReturnsNulloptWhenACellWouldBeSubPixel)
{
    EXPECT_FALSE(
        layoutFor(Size{.width = 3, .height = 40}, 4, 4).has_value());
    EXPECT_FALSE(
        layoutFor(Size{.width = 40, .height = 3}, 4, 4).has_value());
    EXPECT_FALSE(layoutFor(Size{}, 4, 4).has_value());
}

TEST(BoardLayoutTest, CellAt_FindsTheCellAPointFallsIn)
{
    const auto layout = exactLayout();

    EXPECT_EQ(cellAt(layout, 0, 0), (CellCoordinate{.x = 0, .y = 0}));
    EXPECT_EQ(cellAt(layout, 9, 9), (CellCoordinate{.x = 0, .y = 0}));
    EXPECT_EQ(cellAt(layout, 10, 0), (CellCoordinate{.x = 1, .y = 0}));
    EXPECT_EQ(cellAt(layout, 0, 10), (CellCoordinate{.x = 0, .y = 1}));
    EXPECT_EQ(cellAt(layout, 25, 34), (CellCoordinate{.x = 2, .y = 3}));
    EXPECT_EQ(cellAt(layout, 39, 39), (CellCoordinate{.x = 3, .y = 3}));
}

TEST(BoardLayoutTest, CellAt_CountsFromTheBoardsOwnOrigin)
{
    const auto layout = BoardLayout{
        .width = 4,
        .height = 4,
        .cell = 10,
        .origin = {.x = 30, .y = 5}};

    EXPECT_FALSE(cellAt(layout, 29, 5).has_value());
    EXPECT_EQ(cellAt(layout, 30, 5), (CellCoordinate{.x = 0, .y = 0}));
    EXPECT_EQ(cellAt(layout, 69, 44), (CellCoordinate{.x = 3, .y = 3}));
    EXPECT_FALSE(cellAt(layout, 70, 44).has_value());
}

TEST(BoardLayoutTest, CellAt_ReturnsNulloptPastEveryEdge)
{
    const auto layout = exactLayout();

    EXPECT_FALSE(cellAt(layout, -1, 20).has_value());
    EXPECT_FALSE(cellAt(layout, 20, -1).has_value());
    EXPECT_FALSE(cellAt(layout, 40, 20).has_value());
    EXPECT_FALSE(cellAt(layout, 20, 40).has_value());
}

// A pointer may report itself well outside the surface.
// A drag continuing past the window's edge is how.
TEST(BoardLayoutTest, CellAt_SurvivesTheExtremesOfAPosition)
{
    const auto layout = BoardLayout{
        .width = 4,
        .height = 4,
        .cell = 10,
        .origin = {.x = -20, .y = 20}};

    constexpr auto lowest = std::numeric_limits<std::int32_t>::min();
    constexpr auto highest = std::numeric_limits<std::int32_t>::max();

    EXPECT_FALSE(cellAt(layout, lowest, lowest).has_value());
    EXPECT_FALSE(cellAt(layout, highest, highest).has_value());
}

TEST(BoardLayoutTest, CellAt_ReturnsNulloptForALayoutWithNoCells)
{
    EXPECT_FALSE(cellAt(BoardLayout{}, 0, 0).has_value());
}

TEST(BoardLayoutTest, Equality_IsFalseWhenAnyFieldDiffers)
{
    const auto layout = exactLayout();

    EXPECT_EQ(layout, exactLayout());
    EXPECT_NE(layout, (BoardLayout{.width = 5, .height = 4, .cell = 10}));
    EXPECT_NE(layout, (BoardLayout{.width = 4, .height = 5, .cell = 10}));
    EXPECT_NE(layout, (BoardLayout{.width = 4, .height = 4, .cell = 11}));
    EXPECT_NE(layout, (BoardLayout{
        .width = 4,
        .height = 4,
        .cell = 10,
        .origin = {.x = 1, .y = 0}}));
    EXPECT_NE(layout, (BoardLayout{
        .width = 4,
        .height = 4,
        .cell = 10,
        .origin = {.x = 0, .y = 1}}));
}

TEST(BoardLayoutTest, CellCoordinateEquality_ComparesBothCoordinates)
{
    constexpr CellCoordinate cell{.x = 2, .y = 3};

    EXPECT_EQ(cell, (CellCoordinate{.x = 2, .y = 3}));
    EXPECT_NE(cell, (CellCoordinate{.x = 3, .y = 3}));
    EXPECT_NE(cell, (CellCoordinate{.x = 2, .y = 2}));
}
