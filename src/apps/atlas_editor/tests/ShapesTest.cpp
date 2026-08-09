#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Shapes.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::ellipsePixels;
using antwika::atlas_editor::linePixels;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::shapePixels;
using antwika::atlas_editor::Tool;
using antwika::gfx::Size;

namespace
{
    constexpr Size kSheet{.width = 16, .height = 16};

    using Pixels = std::vector<Pixel>;

    [[nodiscard]] bool holds(const Pixels &covered, const Pixel pixel)
    {
        return std::ranges::find(covered, pixel) != covered.end();
    }
}

TEST(ShapesTest, LinePixels_CoversTheOnePixelALineOfNoLengthIsOn)
{
    const Pixels expected{Pixel{.x = 3, .y = 4}};

    EXPECT_EQ(
        linePixels(Pixel{.x = 3, .y = 4}, Pixel{.x = 3, .y = 4}, kSheet),
        expected);
}

TEST(ShapesTest, LinePixels_WalksAnAxisAlignedRunEndToEnd)
{
    const Pixels expected{
        Pixel{.x = 1, .y = 2},
        Pixel{.x = 2, .y = 2},
        Pixel{.x = 3, .y = 2}};

    EXPECT_EQ(
        linePixels(Pixel{.x = 1, .y = 2}, Pixel{.x = 3, .y = 2}, kSheet),
        expected);
}

TEST(ShapesTest, LinePixels_StepsBothWaysAlongADiagonal)
{
    const Pixels expected{
        Pixel{.x = 4, .y = 4},
        Pixel{.x = 3, .y = 5},
        Pixel{.x = 2, .y = 6}};

    EXPECT_EQ(
        linePixels(Pixel{.x = 4, .y = 4}, Pixel{.x = 2, .y = 6}, kSheet),
        expected);
}

TEST(ShapesTest, LinePixels_LeansAlongTheLongerAxisOfAShallowLine)
{
    const auto covered =
        linePixels(Pixel{.x = 0, .y = 0}, Pixel{.x = 4, .y = 1}, kSheet);

    ASSERT_EQ(covered.size(), 5U);
    EXPECT_EQ(covered.front(), (Pixel{.x = 0, .y = 0}));
    EXPECT_EQ(covered.back(), (Pixel{.x = 4, .y = 1}));
}

TEST(ShapesTest, LinePixels_WalksAColumnEndToEnd)
{
    const Pixels expected{
        Pixel{.x = 2, .y = 1},
        Pixel{.x = 2, .y = 2},
        Pixel{.x = 2, .y = 3}};

    EXPECT_EQ(
        linePixels(Pixel{.x = 2, .y = 1}, Pixel{.x = 2, .y = 3}, kSheet),
        expected);
}

TEST(ShapesTest, LinePixels_ClimbsAsReadilyAsItFalls)
{
    const Pixels expected{
        Pixel{.x = 2, .y = 3},
        Pixel{.x = 2, .y = 2},
        Pixel{.x = 2, .y = 1}};

    EXPECT_EQ(
        linePixels(Pixel{.x = 2, .y = 3}, Pixel{.x = 2, .y = 1}, kSheet),
        expected);
}

TEST(ShapesTest, LinePixels_DropsThePixelsThatFallOffTheSheet)
{
    const auto covered = linePixels(
        Pixel{.x = -3, .y = 0}, Pixel{.x = 2, .y = 0}, kSheet);

    const Pixels expected{
        Pixel{.x = 0, .y = 0},
        Pixel{.x = 1, .y = 0},
        Pixel{.x = 2, .y = 0}};

    EXPECT_EQ(covered, expected);
}

TEST(ShapesTest, LinePixels_DropsThePixelsPastTheFarCorner)
{
    const auto covered = linePixels(
        Pixel{.x = 14, .y = 14}, Pixel{.x = 18, .y = 18}, kSheet);

    const Pixels expected{
        Pixel{.x = 14, .y = 14}, Pixel{.x = 15, .y = 15}};

    EXPECT_EQ(covered, expected);
}

TEST(ShapesTest, LinePixels_DropsThePixelsBelowTheSheet)
{
    const auto covered = linePixels(
        Pixel{.x = 3, .y = 14}, Pixel{.x = 3, .y = 18}, kSheet);

    const Pixels expected{
        Pixel{.x = 3, .y = 14}, Pixel{.x = 3, .y = 15}};

    EXPECT_EQ(covered, expected);
}

TEST(ShapesTest, LinePixels_DropsThePixelsAboveTheSheet)
{
    const auto covered = linePixels(
        Pixel{.x = 3, .y = -2}, Pixel{.x = 3, .y = 1}, kSheet);

    const Pixels expected{
        Pixel{.x = 3, .y = 0}, Pixel{.x = 3, .y = 1}};

    EXPECT_EQ(covered, expected);
}

TEST(ShapesTest, EllipsePixels_DrawsTheOneCornerADegenerateBoxHas)
{
    const Pixels expected{Pixel{.x = 5, .y = 5}};

    EXPECT_EQ(
        ellipsePixels(
            Pixel{.x = 5, .y = 5}, Pixel{.x = 5, .y = 5}, kSheet),
        expected);
}

TEST(ShapesTest, EllipsePixels_FillsARowWhenTheBoxIsOnePixelTall)
{
    const Pixels expected{
        Pixel{.x = 2, .y = 7},
        Pixel{.x = 3, .y = 7},
        Pixel{.x = 4, .y = 7}};

    EXPECT_EQ(
        ellipsePixels(
            Pixel{.x = 2, .y = 7}, Pixel{.x = 4, .y = 7}, kSheet),
        expected);
}

TEST(ShapesTest, EllipsePixels_TouchesEachSideOfTheBoxItIsDrawnIn)
{
    const auto covered = ellipsePixels(
        Pixel{.x = 2, .y = 2}, Pixel{.x = 10, .y = 8}, kSheet);

    EXPECT_TRUE(holds(covered, Pixel{.x = 6, .y = 2}));
    EXPECT_TRUE(holds(covered, Pixel{.x = 6, .y = 8}));
    EXPECT_TRUE(holds(covered, Pixel{.x = 2, .y = 5}));
    EXPECT_TRUE(holds(covered, Pixel{.x = 10, .y = 5}));
}

TEST(ShapesTest, EllipsePixels_LeavesTheMiddleOfTheOutlineEmpty)
{
    const auto covered = ellipsePixels(
        Pixel{.x = 2, .y = 2}, Pixel{.x = 10, .y = 8}, kSheet);

    EXPECT_FALSE(holds(covered, Pixel{.x = 6, .y = 5}));
    EXPECT_FALSE(holds(covered, Pixel{.x = 2, .y = 2}));
}

TEST(ShapesTest, EllipsePixels_ReadsACornerPairInEitherOrder)
{
    EXPECT_EQ(
        ellipsePixels(
            Pixel{.x = 10, .y = 8}, Pixel{.x = 2, .y = 2}, kSheet),
        ellipsePixels(
            Pixel{.x = 2, .y = 2}, Pixel{.x = 10, .y = 8}, kSheet));
}

TEST(ShapesTest, EllipsePixels_DropsThePixelsThatFallOffTheSheet)
{
    const auto covered = ellipsePixels(
        Pixel{.x = -6, .y = -6}, Pixel{.x = 4, .y = 4}, kSheet);

    for (const auto pixel : covered)
    {
        EXPECT_GE(pixel.x, 0);
        EXPECT_GE(pixel.y, 0);
    }

    EXPECT_FALSE(covered.empty());
}

TEST(ShapesTest, ShapePixels_DrawsAnEllipseForTheEllipseTool)
{
    EXPECT_EQ(
        shapePixels(
            Tool::Ellipse,
            Pixel{.x = 2, .y = 2},
            Pixel{.x = 10, .y = 8},
            kSheet),
        ellipsePixels(
            Pixel{.x = 2, .y = 2}, Pixel{.x = 10, .y = 8}, kSheet));
}

TEST(ShapesTest, ShapePixels_DrawsALineForTheLineTool)
{
    EXPECT_EQ(
        shapePixels(
            Tool::Line,
            Pixel{.x = 2, .y = 2},
            Pixel{.x = 10, .y = 8},
            kSheet),
        linePixels(
            Pixel{.x = 2, .y = 2}, Pixel{.x = 10, .y = 8}, kSheet));
}
