#include <gtest/gtest.h>

#include <antwika/gfx/Rect.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardLayout.hpp>

using antwika::gfx::Rect;
using antwika::sudoku::BoardLayout;
using antwika::sudoku::cellAt;
using antwika::sudoku::layoutFor;
using antwika::sudoku::Square;
using antwika::sudoku::squareRect;

namespace
{
    constexpr Rect kSquareArea{
        .origin = {.x = 0, .y = 100},
        .size = {.width = 450, .height = 450}};

    TEST(BoardLayoutTest, LayoutFor_GivesEverySquareAWholeNumberOfPixels)
    {
        const auto layout = layoutFor(kSquareArea);

        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->cell, 50U);
        EXPECT_EQ(layout->origin, (antwika::gfx::Point{.x = 0, .y = 100}));
    }

    TEST(BoardLayoutTest, LayoutFor_CentresTheGridInWhatIsLeftOver)
    {
        const auto layout = layoutFor(Rect{
            .origin = {.x = 10, .y = 20},
            .size = {.width = 500, .height = 452}});

        ASSERT_TRUE(layout.has_value());

        EXPECT_EQ(layout->cell, 50U);
        EXPECT_EQ(layout->origin, (antwika::gfx::Point{.x = 35, .y = 21}));
    }

    TEST(BoardLayoutTest, LayoutFor_RefusesAnAreaWithNoPixelPerSquare)
    {
        EXPECT_FALSE(layoutFor(Rect{}).has_value());
        EXPECT_FALSE(
            layoutFor(
                Rect{
                    .origin = {},
                    .size = {.width = 8, .height = 400}})
                .has_value());
    }

    TEST(BoardLayoutTest, CellAt_FindsTheSquareUnderAPoint)
    {
        const auto layout = layoutFor(kSquareArea).value();

        EXPECT_EQ(
            cellAt(layout, 25, 125),
            (std::optional{Square{.row = 0, .col = 0}}));
        EXPECT_EQ(
            cellAt(layout, 449, 549),
            (std::optional{Square{.row = 8, .col = 8}}));
        EXPECT_EQ(
            cellAt(layout, 175, 275),
            (std::optional{Square{.row = 3, .col = 3}}));
    }

    TEST(BoardLayoutTest, CellAt_MissesEverythingOutsideTheGrid)
    {
        const auto layout = layoutFor(kSquareArea).value();

        EXPECT_FALSE(cellAt(layout, -1, 125).has_value());
        EXPECT_FALSE(cellAt(layout, 25, 99).has_value());
        EXPECT_FALSE(cellAt(layout, 450, 125).has_value());
        EXPECT_FALSE(cellAt(layout, 25, 550).has_value());
    }

    TEST(BoardLayoutTest, CellAt_MissesEverythingForALayoutWithNoGrid)
    {
        EXPECT_FALSE(cellAt(BoardLayout{}, 0, 0).has_value());
    }

    TEST(BoardLayoutTest, SquareRect_PlacesASquareWhereCellAtFindsIt)
    {
        const auto layout = layoutFor(kSquareArea).value();
        const Square square{.row = 2, .col = 5};
        const auto rect = squareRect(layout, square);

        EXPECT_EQ(rect.origin, (antwika::gfx::Point{.x = 250, .y = 200}));
        EXPECT_EQ(rect.size.width, 50U);
        EXPECT_EQ(rect.size.height, 50U);

        EXPECT_EQ(
            cellAt(layout, rect.origin.x + 1, rect.origin.y + 1),
            std::optional{square});
    }
}
