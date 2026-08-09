#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::Viewport;
using antwika::gfx::viewportFor;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};
}

TEST(ViewportTest, Default_IsTheIdentity)
{
    const Viewport viewport;

    EXPECT_EQ(viewport.toWindow(Point{.x = 7, .y = 9}), (Point{7, 9}));
    EXPECT_EQ(viewport.toCanvas(Point{.x = 7, .y = 9}), (Point{7, 9}));
    EXPECT_EQ(viewport.toWindowScale(3), 3u);
}

TEST(ViewportTest, ViewportFor_IsTheIdentityWhenTheSizesMatch)
{
    EXPECT_EQ(viewportFor(kCanvas, kCanvas), Viewport{});
}

TEST(ViewportTest, ViewportFor_IsTheIdentityForADegenerateSize)
{
    const Size none{.width = 0, .height = 0};

    EXPECT_EQ(
        viewportFor(Size{.width = 0, .height = 640}, kCanvas), Viewport{});
    EXPECT_EQ(
        viewportFor(Size{.width = 1024, .height = 0}, kCanvas), Viewport{});
    EXPECT_EQ(viewportFor(kCanvas, Size{.width = 0, .height = 64}), Viewport{});
    EXPECT_EQ(viewportFor(kCanvas, Size{.width = 64, .height = 0}), Viewport{});
    EXPECT_EQ(viewportFor(none, none), Viewport{});
}

TEST(ViewportTest, ViewportFor_ScalesByHeightWhateverTheWidthIs)
{
    const auto wide =
        viewportFor(Size{.width = 2560, .height = 1280}, kCanvas);
    const auto narrow =
        viewportFor(Size{.width = 2100, .height = 1280}, kCanvas);

    EXPECT_EQ(wide.frame(kCanvas).size, (Size{2048, 1280}));
    EXPECT_EQ(narrow.frame(kCanvas).size, (Size{2048, 1280}));

    EXPECT_EQ(wide.offset, (Point{256, 0}));
    EXPECT_EQ(narrow.offset, (Point{26, 0}));
}

TEST(ViewportTest, ViewportFor_PillarboxesAWindowWiderThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 1920, .height = 640}, kCanvas);

    EXPECT_EQ(viewport.frame(kCanvas).size, kCanvas);
    EXPECT_EQ(viewport.offset, (Point{448, 0}));
}

TEST(ViewportTest, ViewportFor_LetterboxesAWindowNarrowerThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 512, .height = 640}, kCanvas);

    EXPECT_EQ(viewport.frame(kCanvas).size, (Size{512, 320}));
    EXPECT_EQ(viewport.offset, (Point{0, 160}));
}

TEST(ViewportTest, ViewportFor_FitsAWindowSmallerThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 256, .height = 320}, kCanvas);

    const auto frame = viewport.frame(kCanvas);

    EXPECT_LE(frame.size.width, 256u);
    EXPECT_LE(frame.size.height, 320u);
    EXPECT_GE(viewport.offset.x, 0);
    EXPECT_GE(viewport.offset.y, 0);
}

TEST(ViewportTest, ToWindow_ScalesAndOffsetsAPoint)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvas);

    EXPECT_EQ(viewport.toWindow(Point{.x = 0, .y = 0}), (Point{0, 0}));
    EXPECT_EQ(viewport.toWindow(Point{.x = 10, .y = 20}), (Point{20, 40}));
}

TEST(ViewportTest, ToWindow_KeepsTwoTouchingRectanglesTouching)
{
    const auto viewport =
        viewportFor(Size{.width = 1536, .height = 960}, kCanvas);

    const auto left = viewport.toWindow(
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 7, .height = 4}});
    const auto right = viewport.toWindow(
        Rect{.origin = {.x = 7, .y = 0}, .size = {.width = 5, .height = 4}});

    EXPECT_EQ(
        left.origin.x + static_cast<std::int32_t>(left.size.width),
        right.origin.x);
}

TEST(ViewportTest, ToCanvas_UndoesToWindow)
{
    const auto viewport =
        viewportFor(Size{.width = 2560, .height = 1280}, kCanvas);

    ASSERT_EQ(viewport.toWindow(Point{.x = 10, .y = 20}), (Point{276, 40}));

    for (std::int32_t x = 0; x < 1024; x += 37)
    {
        const Point on{.x = x, .y = x % 640};

        EXPECT_EQ(viewport.toCanvas(viewport.toWindow(on)), on) << x;
    }
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasForAPointOnABar)
{
    const auto viewport =
        viewportFor(Size{.width = 1920, .height = 640}, kCanvas);

    const auto onBar = viewport.toCanvas(Point{.x = 100, .y = 300});

    EXPECT_LT(onBar.x, 0);
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasOnePixelIntoAPillar)
{
    const auto viewport =
        viewportFor(Size{.width = 2148, .height = 1280}, kCanvas);

    ASSERT_EQ(viewport.offset, (Point{50, 0}));

    EXPECT_EQ(viewport.toCanvas(Point{.x = 50, .y = 0}).x, 0);
    EXPECT_EQ(viewport.toCanvas(Point{.x = 49, .y = 0}).x, -1);
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasOnePixelIntoALetterbox)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1380}, kCanvas);

    ASSERT_EQ(viewport.offset, (Point{0, 50}));

    EXPECT_EQ(viewport.toCanvas(Point{.x = 0, .y = 50}).y, 0);
    EXPECT_EQ(viewport.toCanvas(Point{.x = 0, .y = 49}).y, -1);
}

TEST(ViewportTest, ToWindowScale_LeavesAScaleOfZeroAlone)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvas);

    EXPECT_EQ(viewport.toWindowScale(0), 0u);
}

TEST(ViewportTest, ToWindowScale_GrowsWithTheViewport)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvas);

    EXPECT_EQ(viewport.toWindowScale(1), 2u);
    EXPECT_EQ(viewport.toWindowScale(3), 6u);
}

TEST(ViewportTest, ToWindowScale_NeverRoundsRealTextAwayToNothing)
{
    const auto viewport =
        viewportFor(Size{.width = 256, .height = 160}, kCanvas);

    EXPECT_EQ(viewport.toWindowScale(1), 1u);
    EXPECT_EQ(viewport.toWindowScale(2), 1u);
}

TEST(ViewportTest, OperatorEquals_ComparesEveryField)
{
    const Viewport base{
        .offset = {.x = 1, .y = 2}, .numerator = 3, .denominator = 4};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto moved = base;
    moved.offset.x = 9;
    EXPECT_NE(base, moved);

    auto stretched = base;
    stretched.numerator = 6;
    EXPECT_NE(base, stretched);

    auto squashed = base;
    squashed.denominator = 8;
    EXPECT_NE(base, squashed);
}
