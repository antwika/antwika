#include <gtest/gtest.h>

#include <algorithm>

#include <cstddef>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"

using antwika::atlas_editor::CanvasView;
using antwika::atlas_editor::centredView;
using antwika::atlas_editor::imageRect;
using antwika::atlas_editor::kZoomScales;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::pixelAt;
using antwika::atlas_editor::pixelRect;
using antwika::atlas_editor::pannedBy;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::zoomedIn;
using antwika::atlas_editor::zoomedOut;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

TEST(CanvasViewTest, OperatorEquals_ComparesThePanAndTheZoomLevel)
{
    const CanvasView view{.pan = {.x = 3, .y = 4}, .zoom = 2};

    EXPECT_EQ(view, (CanvasView{.pan = {.x = 3, .y = 4}, .zoom = 2}));
    EXPECT_NE(view, (CanvasView{.pan = {.x = 9, .y = 4}, .zoom = 2}));
    EXPECT_NE(view, (CanvasView{.pan = {.x = 3, .y = 4}, .zoom = 5}));
}

TEST(PixelTest, OperatorEquals_ComparesBothCoordinates)
{
    const Pixel pixel{.x = 3, .y = 4};

    EXPECT_EQ(pixel, (Pixel{.x = 3, .y = 4}));
    EXPECT_NE(pixel, (Pixel{.x = 9, .y = 4}));
    EXPECT_NE(pixel, (Pixel{.x = 3, .y = 9}));
}

TEST(CanvasViewTest, ScaleOf_IsTheTableEntryForTheLevel)
{
    EXPECT_EQ(scaleOf(CanvasView{}), kZoomScales.front());
    EXPECT_EQ(scaleOf(CanvasView{.pan = {}, .zoom = 3}), kZoomScales[3]);
}

TEST(CanvasViewTest, ScaleOf_ReadsALevelPastTheTableAsTheClosestOne)
{
    EXPECT_EQ(
        scaleOf(CanvasView{.pan = {}, .zoom = 99}), kZoomScales.back());
}

TEST(CanvasViewTest, CentredView_LeavesEqualMarginsOnBothSides)
{
    const auto view = centredView(
        Size{.width = 100, .height = 60}, Size{.width = 40, .height = 20},
        0);

    EXPECT_EQ(view.pan, (Point{.x = 30, .y = 20}));
    EXPECT_EQ(view.zoom, 0U);
}

TEST(CanvasViewTest, CentredView_PutsASheetWiderThanTheCanvasAtANegativePan)
{
    const auto view = centredView(
        Size{.width = 100, .height = 60}, Size{.width = 40, .height = 20},
        3);

    EXPECT_EQ(view.pan.x, -30);
}

TEST(CanvasViewTest, PixelAt_FloorsRatherThanTruncates)
{
    const CanvasView view{.pan = {.x = 10, .y = 10}, .zoom = 1};

    EXPECT_EQ(pixelAt(view, Point{.x = 10, .y = 10}), (Pixel{}));
    EXPECT_EQ(
        pixelAt(view, Point{.x = 11, .y = 13}),
        (Pixel{.x = 0, .y = 1}));

    EXPECT_EQ(
        pixelAt(view, Point{.x = 9, .y = 8}),
        (Pixel{.x = -1, .y = -1}));

    EXPECT_EQ(
        pixelAt(view, Point{.x = 8, .y = 6}),
        (Pixel{.x = -1, .y = -2}));
}

TEST(CanvasViewTest, PixelRect_IsWhatPixelAtWouldMapBack)
{
    const CanvasView view{.pan = {.x = 4, .y = 6}, .zoom = 2};

    EXPECT_EQ(
        pixelRect(view, Pixel{.x = 2, .y = 1}),
        (Rect{
            .origin = {.x = 10, .y = 9},
            .size = {.width = 3, .height = 3}}));
    EXPECT_EQ(
        pixelAt(view, Point{.x = 10, .y = 9}), (Pixel{.x = 2, .y = 1}));
}

TEST(CanvasViewTest, ImageRect_CoversEveryPixelOfTheSheet)
{
    const CanvasView view{.pan = {.x = -5, .y = 2}, .zoom = 1};

    EXPECT_EQ(
        imageRect(view, Size{.width = 8, .height = 4}),
        (Rect{
            .origin = {.x = -5, .y = 2},
            .size = {.width = 16, .height = 8}}));
}

TEST(CanvasViewTest, ZoomedIn_KeepsThePixelUnderTheAnchorUnderIt)
{
    const CanvasView view{.pan = {.x = 0, .y = 0}, .zoom = 0};
    const Point anchor{.x = 40, .y = 24};

    const auto zoomed = zoomedIn(view, anchor);

    EXPECT_EQ(zoomed.zoom, 1U);
    EXPECT_EQ(pixelAt(zoomed, anchor), pixelAt(view, anchor));
}

TEST(CanvasViewTest, ZoomedOut_KeepsThePixelUnderTheAnchorUnderIt)
{
    const CanvasView view{.pan = {.x = 13, .y = -7}, .zoom = 4};
    const Point anchor{.x = 200, .y = 130};

    const auto zoomed = zoomedOut(view, anchor);

    EXPECT_EQ(zoomed.zoom, 3U);
    EXPECT_EQ(pixelAt(zoomed, anchor), pixelAt(view, anchor));
}

TEST(CanvasViewTest, ZoomedIn_StopsAtTheClosestLevel)
{
    const CanvasView view{
        .pan = {}, .zoom = kZoomScales.size() - 1};

    EXPECT_EQ(zoomedIn(view, Point{.x = 3, .y = 3}), view);
}

TEST(CanvasViewTest, ZoomedOut_StopsAtTheWidestLevel)
{
    const CanvasView view{.pan = {.x = 2, .y = 2}, .zoom = 0};

    EXPECT_EQ(zoomedOut(view, Point{.x = 3, .y = 3}), view);
}

TEST(CanvasViewTest, PannedBy_MovesThePanAndLeavesTheZoom)
{
    const CanvasView view{.pan = {.x = 5, .y = 5}, .zoom = 2};

    EXPECT_EQ(
        pannedBy(view, Point{.x = -8, .y = 3}),
        (CanvasView{.pan = {.x = -3, .y = 8}, .zoom = 2}));
}

TEST(CanvasViewTest, ScaleOf_ReachesThirtyTwoTimesTheSheet)
{
    EXPECT_EQ(kZoomScales.back(), 32U);
    EXPECT_TRUE(std::ranges::is_sorted(kZoomScales));
}
