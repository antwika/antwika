#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

using antwika::atlas_editor::blitFor;
using antwika::atlas_editor::CanvasView;
using antwika::atlas_editor::fittedView;
using antwika::atlas_editor::kZoomScales;
using antwika::atlas_editor::paneHolds;
using antwika::atlas_editor::PreviewPane;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::viewOfSlot;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr Rect kPane{
        .origin = {.x = 100, .y = 50},
        .size = {.width = 200, .height = 160}};

    constexpr Size kSheet{.width = 64, .height = 64};

    [[nodiscard]] bool inside(const Rect &outer, const Rect &inner)
    {
        return inner.origin.x >= outer.origin.x
               && inner.origin.y >= outer.origin.y
               && inner.origin.x + static_cast<std::int32_t>(
                      inner.size.width)
                      <= outer.origin.x
                             + static_cast<std::int32_t>(outer.size.width)
               && inner.origin.y + static_cast<std::int32_t>(
                      inner.size.height)
                      <= outer.origin.y
                             + static_cast<std::int32_t>(
                                 outer.size.height);
    }
}

TEST(PreviewTest, PaneHolds_TakesTheTopLeftAndNotTheBottomRight)
{
    EXPECT_TRUE(paneHolds(kPane, Point{.x = 100, .y = 50}));
    EXPECT_TRUE(paneHolds(kPane, Point{.x = 299, .y = 209}));
    EXPECT_FALSE(paneHolds(kPane, Point{.x = 300, .y = 209}));
    EXPECT_FALSE(paneHolds(kPane, Point{.x = 299, .y = 210}));
    EXPECT_FALSE(paneHolds(kPane, Point{.x = 99, .y = 50}));
    EXPECT_FALSE(paneHolds(kPane, Point{.x = 100, .y = 49}));
}

TEST(PreviewTest, BlitFor_ShowsTheWholeSheetWhenItFits)
{
    const CanvasView view{.pan = {.x = 100, .y = 50}, .zoom = 0};

    const auto blit = blitFor(view, kPane, kSheet);

    ASSERT_TRUE(blit.has_value());
    EXPECT_EQ(blit->source.origin, (Point{.x = 0, .y = 0}));
    EXPECT_EQ(blit->source.size, kSheet);
    EXPECT_EQ(blit->destination.origin, (Point{.x = 100, .y = 50}));
    EXPECT_EQ(blit->destination.size, kSheet);
}

TEST(PreviewTest, BlitFor_KeepsWhatItDrawsInsideThePane)
{
    for (std::size_t zoom = 0; zoom < kZoomScales.size(); ++zoom)
    {
        for (std::int32_t nudge = -7; nudge <= 7; ++nudge)
        {
            const CanvasView view{
                .pan = {.x = 90 + nudge, .y = 40 + nudge}, .zoom = zoom};

            const auto blit = blitFor(view, kPane, kSheet);

            if (!blit.has_value())
            {
                continue;
            }

            EXPECT_TRUE(inside(kPane, blit->destination))
                << zoom << ' ' << nudge;
        }
    }
}

TEST(PreviewTest, BlitFor_ScalesTheSourceByTheViewsScale)
{
    const CanvasView view{.pan = {.x = 100, .y = 50}, .zoom = 3};
    const auto scale = scaleOf(view);

    const auto blit = blitFor(view, kPane, kSheet);

    ASSERT_TRUE(blit.has_value());
    EXPECT_EQ(blit->destination.size.width, blit->source.size.width * scale);
    EXPECT_EQ(
        blit->destination.size.height, blit->source.size.height * scale);
}

TEST(PreviewTest, BlitFor_SeesNothingWhenTheViewIsOffTheSheet)
{
    const CanvasView view{.pan = {.x = 1000, .y = 1000}, .zoom = 0};

    EXPECT_FALSE(blitFor(view, kPane, kSheet).has_value());
}

TEST(PreviewTest, BlitFor_SeesNothingWhenNoWholePixelFits)
{
    constexpr Rect kSliver{
        .origin = {.x = 100, .y = 50},
        .size = {.width = 4, .height = 4}};

    const CanvasView view{
        .pan = {.x = 98, .y = 48},
        .zoom = kZoomScales.size() - 1};

    EXPECT_FALSE(blitFor(view, kSliver, kSheet).has_value());
}

TEST(PreviewTest, FittedView_PicksTheLargestScaleTheSlotFitsAt)
{
    constexpr Rect kSlot{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 16, .height = 16}};

    const auto view = fittedView(kPane, kSlot);

    EXPECT_EQ(scaleOf(view), 8U);
}

TEST(PreviewTest, FittedView_CentresTheSlotInThePane)
{
    constexpr Rect kSlot{
        .origin = {.x = 32, .y = 16},
        .size = {.width = 16, .height = 16}};

    const auto view = fittedView(kPane, kSlot);
    const auto scale = scaleOf(view);

    const Point corner{
        .x = view.pan.x + static_cast<std::int32_t>(kSlot.origin.x * scale),
        .y = view.pan.y
             + static_cast<std::int32_t>(kSlot.origin.y * scale)};

    EXPECT_EQ(
        corner.x - kPane.origin.x,
        static_cast<std::int32_t>(
            (kPane.size.width - kSlot.size.width * scale) / 2));
    EXPECT_EQ(
        corner.y - kPane.origin.y,
        static_cast<std::int32_t>(
            (kPane.size.height - kSlot.size.height * scale) / 2));
}

TEST(PreviewTest, FittedView_FallsBackToTheSmallestScaleForAHugeSlot)
{
    constexpr Rect kSlot{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 4000, .height = 4000}};

    EXPECT_EQ(scaleOf(fittedView(kPane, kSlot)), 1U);
}

TEST(PreviewTest, ViewOfSlot_FramesTheSlotTheIndexNames)
{
    constexpr TileGrid kTiles{.width = 16, .height = 16};

    const auto view = viewOfSlot(kPane, kTiles, kSheet, 5);

    ASSERT_TRUE(view.has_value());
    EXPECT_EQ(
        *view,
        fittedView(
            kPane,
            Rect{
                .origin = {.x = 16, .y = 16},
                .size = {.width = 16, .height = 16}}));
}

TEST(PreviewTest, ViewOfSlot_FramesNothingForASlotPastTheGrid)
{
    constexpr TileGrid kTiles{.width = 16, .height = 16};

    EXPECT_FALSE(viewOfSlot(kPane, kTiles, kSheet, 16).has_value());
}

TEST(PreviewTest, ViewOfSlot_FramesNothingWhenTheGridHasNoColumns)
{
    constexpr TileGrid kTiles{.width = 0, .height = 16};

    EXPECT_FALSE(viewOfSlot(kPane, kTiles, kSheet, 0).has_value());
}

TEST(PreviewTest, OperatorEquals_ComparesEveryFieldOfThePane)
{
    constexpr PreviewPane base{};

    PreviewPane other = base;
    other.open = !base.open;
    EXPECT_NE(base, other);

    other = base;
    other.autoFocus = !base.autoFocus;
    EXPECT_NE(base, other);

    other = base;
    other.ratio = base.ratio + 1;
    EXPECT_NE(base, other);

    other = base;
    other.dragging = !base.dragging;
    EXPECT_NE(base, other);

    other = base;
    other.view.zoom = base.view.zoom + 1;
    EXPECT_NE(base, other);

    other = base;
    other.focused = 3;
    EXPECT_NE(base, other);

    EXPECT_EQ(base, PreviewPane{base});
}

TEST(PreviewTest, BlitFor_SeesNothingWhenThePaneSitsAboveTheSheet)
{
    const CanvasView view{.pan = {.x = 100, .y = 400}, .zoom = 0};

    EXPECT_FALSE(blitFor(view, kPane, kSheet).has_value());
}

TEST(PreviewTest, BlitFor_SeesNothingWhenNoWholeRowFits)
{
    constexpr Rect kBand{
        .origin = {.x = 100, .y = 50},
        .size = {.width = 200, .height = 4}};

    const CanvasView view{
        .pan = {.x = 100, .y = 48},
        .zoom = kZoomScales.size() - 1};

    EXPECT_FALSE(blitFor(view, kBand, kSheet).has_value());
}

TEST(PreviewTest, FittedView_TakesTheHeightIntoAccountAsWellAsTheWidth)
{
    constexpr Rect kWide{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 1000, .height = 40}};

    constexpr Rect kSlot{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 8, .height = 8}};

    EXPECT_EQ(scaleOf(fittedView(kWide, kSlot)), 4U);
}

TEST(PreviewTest, BlitFor_SeesNothingWhenNoWholeColumnFits)
{
    constexpr Rect kColumn{
        .origin = {.x = 100, .y = 50},
        .size = {.width = 4, .height = 200}};

    const CanvasView view{
        .pan = {.x = 98, .y = 50},
        .zoom = kZoomScales.size() - 1};

    EXPECT_FALSE(blitFor(view, kColumn, kSheet).has_value());
}
