#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <antwika/app/FramePacing.hpp>
#include <antwika/camera/FlyCamera.hpp>

#include "antwika/editor/editor/state/PanelSizes.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"

using antwika::editor::getFittedPanelWidth;
using antwika::editor::getRailWidth;
using antwika::editor::getRailWidthOnCanvas;
using antwika::editor::kMinPanelWidth;
using antwika::editor::kRightPanelWidth;
using antwika::editor::PanelSizes;

namespace
{
    constexpr auto kWindowSize = antwika::app::kDefaultWindowSize;

    constexpr auto kCanvasSize = antwika::camera::kCanvasSize;

    constexpr std::uint32_t kRestingWidth = 170;

    constexpr std::array<std::uint32_t PanelSizes::*, 7> kEveryExtent{
        &PanelSizes::toolWidth,
        &PanelSizes::entityWidth,
        &PanelSizes::inspectWidth,
        &PanelSizes::railWidth,
        &PanelSizes::cardWidth,
        &PanelSizes::planFirstWidth,
        &PanelSizes::planSecondWidth};
}

TEST(PanelSizesTest, FittedPanelWidth_RestsWhereNothingWasDragged)
{
    EXPECT_EQ(
        getFittedPanelWidth(0, kRestingWidth, kWindowSize.width),
        kRestingWidth);
}

TEST(PanelSizesTest, FittedPanelWidth_PassesAWidthThatAlreadyFits)
{
    EXPECT_EQ(
        getFittedPanelWidth(200, kRestingWidth, kWindowSize.width), 200U);
}

TEST(PanelSizesTest, FittedPanelWidth_HoldsANarrowWishAtTheMinimum)
{
    EXPECT_EQ(
        getFittedPanelWidth(1, kRestingWidth, kWindowSize.width),
        kMinPanelWidth);
}

TEST(PanelSizesTest, FittedPanelWidth_HoldsAWideWishToAThirdOfTheWindow)
{
    EXPECT_EQ(
        getFittedPanelWidth(4000, kRestingWidth, kWindowSize.width),
        kWindowSize.width / 3);
}

TEST(PanelSizesTest, FittedPanelWidth_KeepsTheMinimumOnATinyWindow)
{
    EXPECT_EQ(
        getFittedPanelWidth(200, kRestingWidth, 60), kMinPanelWidth);
}

TEST(PanelSizesTest, RailWidthOnCanvas_RestsOnTheOldConstantUntilDragged)
{
    EXPECT_FLOAT_EQ(
        getRailWidthOnCanvas(PanelSizes{}, kWindowSize, kCanvasSize),
        kRightPanelWidth);
}

TEST(PanelSizesTest, RailWidthOnCanvas_RestsOnTheOldConstantWithNoWindow)
{
    PanelSizes sizes;
    sizes.railWidth = 200;

    EXPECT_FLOAT_EQ(
        getRailWidthOnCanvas(
            sizes, antwika::gfx::Size{.width = 0, .height = 0}, kCanvasSize),
        kRightPanelWidth);
}

TEST(PanelSizesTest, RailWidthOnCanvas_CarriesADraggedWidthIntoCanvasUnits)
{
    PanelSizes sizes;
    sizes.railWidth = 240;

    EXPECT_FLOAT_EQ(
        getRailWidthOnCanvas(sizes, kWindowSize, kCanvasSize), 90.0F);
}

TEST(PanelSizesTest, RailWidthOnCanvas_HoldsAWideWishBeforeTheCanvasStep)
{
    PanelSizes sizes;
    sizes.railWidth = 1200;

    EXPECT_FLOAT_EQ(
        getRailWidthOnCanvas(sizes, kWindowSize, kCanvasSize), 159.75F);
}

TEST(PanelSizesTest, PanelSizes_RestUnsetSoTheOldWidthsStillDecide)
{
    constexpr PanelSizes sizes;

    for (const auto extent : kEveryExtent)
    {
        EXPECT_EQ(sizes.*extent, 0U);
    }
}

TEST(PanelSizesTest, OperatorEquals_ComparesEveryWidth)
{
    constexpr PanelSizes baseSizes;

    EXPECT_EQ(baseSizes, PanelSizes{});

    for (const auto extent : kEveryExtent)
    {
        PanelSizes otherSizes;
        otherSizes.*extent = 1;

        EXPECT_NE(baseSizes, otherSizes);
    }
}
