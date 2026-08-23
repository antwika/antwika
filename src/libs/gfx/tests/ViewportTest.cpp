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
    constexpr Size kCanvasSize{.width = 1024, .height = 640};
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
    EXPECT_EQ(viewportFor(kCanvasSize, kCanvasSize), Viewport{});
}

TEST(ViewportTest, ViewportFor_IsTheIdentityForADegenerateSize)
{
    const Size noneSize{.width = 0, .height = 0};

    EXPECT_EQ(
        viewportFor(Size{.width = 0, .height = 640}, kCanvasSize), Viewport{});
    EXPECT_EQ(
        viewportFor(Size{.width = 1024, .height = 0}, kCanvasSize), Viewport{});
    EXPECT_EQ(
        viewportFor(kCanvasSize, Size{.width = 0, .height = 64}),
        Viewport{});
    EXPECT_EQ(
        viewportFor(kCanvasSize, Size{.width = 64, .height = 0}),
        Viewport{});
    EXPECT_EQ(viewportFor(noneSize, noneSize), Viewport{});
}

TEST(ViewportTest, ViewportFor_ScalesByHeightWhateverTheWidthIs)
{
    const auto wideViewport =
        viewportFor(Size{.width = 2560, .height = 1280}, kCanvasSize);
    const auto narrow =
        viewportFor(Size{.width = 2100, .height = 1280}, kCanvasSize);

    EXPECT_EQ(wideViewport.getFrame(kCanvasSize).size, (Size{2048, 1280}));
    EXPECT_EQ(narrow.getFrame(kCanvasSize).size, (Size{2048, 1280}));

    EXPECT_EQ(wideViewport.offsetPoint, (Point{256, 0}));
    EXPECT_EQ(narrow.offsetPoint, (Point{26, 0}));
}

TEST(ViewportTest, ViewportFor_PillarboxesAWindowWiderThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 1920, .height = 640}, kCanvasSize);

    EXPECT_EQ(viewport.getFrame(kCanvasSize).size, kCanvasSize);
    EXPECT_EQ(viewport.offsetPoint, (Point{448, 0}));
}

TEST(ViewportTest, ViewportFor_LetterboxesAWindowNarrowerThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 512, .height = 640}, kCanvasSize);

    EXPECT_EQ(viewport.getFrame(kCanvasSize).size, (Size{512, 320}));
    EXPECT_EQ(viewport.offsetPoint, (Point{0, 160}));
}

TEST(ViewportTest, ViewportFor_FitsAWindowSmallerThanTheCanvas)
{
    const auto viewport =
        viewportFor(Size{.width = 256, .height = 320}, kCanvasSize);

    const auto frame = viewport.getFrame(kCanvasSize);

    EXPECT_LE(frame.size.width, 256u);
    EXPECT_LE(frame.size.height, 320u);
    EXPECT_GE(viewport.offsetPoint.x, 0);
    EXPECT_GE(viewport.offsetPoint.y, 0);
}

TEST(ViewportTest, ToWindow_ScalesAndOffsetsAPoint)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvasSize);

    EXPECT_EQ(viewport.toWindow(Point{.x = 0, .y = 0}), (Point{0, 0}));
    EXPECT_EQ(viewport.toWindow(Point{.x = 10, .y = 20}), (Point{20, 40}));
}

TEST(ViewportTest, ToWindow_KeepsTwoTouchingRectanglesTouching)
{
    const auto viewport =
        viewportFor(Size{.width = 1536, .height = 960}, kCanvasSize);

    const auto left = viewport.toWindow(
        Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 7, .height = 4}});
    const auto right = viewport.toWindow(
        Rect{
            .originPoint = {.x = 7, .y = 0},
            .size = {.width = 5, .height = 4}});

    EXPECT_EQ(
        left.originPoint.x + static_cast<std::int32_t>(left.size.width),
        right.originPoint.x);
}

TEST(ViewportTest, ToCanvas_UndoesToWindow)
{
    const auto viewport =
        viewportFor(Size{.width = 2560, .height = 1280}, kCanvasSize);

    ASSERT_EQ(viewport.toWindow(Point{.x = 10, .y = 20}), (Point{276, 40}));

    for (std::int32_t x = 0; x < 1024; x += 37)
    {
        const Point onPoint{.x = x, .y = x % 640};

        EXPECT_EQ(viewport.toCanvas(viewport.toWindow(onPoint)), onPoint) << x;
    }
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasForAPointOnABar)
{
    const auto viewport =
        viewportFor(Size{.width = 1920, .height = 640}, kCanvasSize);

    const auto onBar = viewport.toCanvas(Point{.x = 100, .y = 300});

    EXPECT_LT(onBar.x, 0);
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasOnePixelIntoAPillar)
{
    const auto viewport =
        viewportFor(Size{.width = 2148, .height = 1280}, kCanvasSize);

    ASSERT_EQ(viewport.offsetPoint, (Point{50, 0}));

    EXPECT_EQ(viewport.toCanvas(Point{.x = 50, .y = 0}).x, 0);
    EXPECT_EQ(viewport.toCanvas(Point{.x = 49, .y = 0}).x, -1);
}

TEST(ViewportTest, ToCanvas_AnswersOutsideTheCanvasOnePixelIntoALetterbox)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1380}, kCanvasSize);

    ASSERT_EQ(viewport.offsetPoint, (Point{0, 50}));

    EXPECT_EQ(viewport.toCanvas(Point{.x = 0, .y = 50}).y, 0);
    EXPECT_EQ(viewport.toCanvas(Point{.x = 0, .y = 49}).y, -1);
}

TEST(ViewportTest, ToWindowScale_LeavesAScaleOfZeroAlone)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvasSize);

    EXPECT_EQ(viewport.toWindowScale(0), 0u);
}

TEST(ViewportTest, ToWindowScale_GrowsWithTheViewport)
{
    const auto viewport =
        viewportFor(Size{.width = 2048, .height = 1280}, kCanvasSize);

    EXPECT_EQ(viewport.toWindowScale(1), 2u);
    EXPECT_EQ(viewport.toWindowScale(3), 6u);
}

TEST(ViewportTest, ToWindowScale_NeverRoundsRealTextAwayToNothing)
{
    const auto viewport =
        viewportFor(Size{.width = 256, .height = 160}, kCanvasSize);

    EXPECT_EQ(viewport.toWindowScale(1), 1u);
    EXPECT_EQ(viewport.toWindowScale(2), 1u);
}

TEST(ViewportTest, OperatorEquals_ComparesEveryField)
{
    const Viewport baseViewport{
        .offsetPoint = {.x = 1, .y = 2}, .numerator = 3, .denominator = 4};

    const auto twin = baseViewport;
    EXPECT_EQ(baseViewport, twin);

    auto movedViewport = baseViewport;
    movedViewport.offsetPoint.x = 9;
    EXPECT_NE(baseViewport, movedViewport);

    auto stretchedViewport = baseViewport;
    stretchedViewport.numerator = 6;
    EXPECT_NE(baseViewport, stretchedViewport);

    auto squashedViewport = baseViewport;
    squashedViewport.denominator = 8;
    EXPECT_NE(baseViewport, squashedViewport);
}

TEST(ViewportTest, ViewportFor_ScalesByAWholeNumberWhenAskedTo)
{
    const auto viewport = viewportFor(
        Size{.width = 1280, .height = 720},
        Size{.width = 480, .height = 270},
        antwika::gfx::Fit::IntegerScale);

    EXPECT_EQ(viewport.numerator, 2U);
    EXPECT_EQ(viewport.denominator, 1U);
    EXPECT_EQ(viewport.offsetPoint.x, 160);
    EXPECT_EQ(viewport.offsetPoint.y, 90);
}

TEST(ViewportTest, ViewportFor_StretchesByThePartsItMustToFill)
{
    const auto viewport = viewportFor(
        Size{.width = 1280, .height = 720},
        Size{.width = 480, .height = 270});

    EXPECT_EQ(viewport.numerator, 8U);
    EXPECT_EQ(viewport.denominator, 3U);
}

TEST(ViewportTest, ViewportFor_KeepsAnIntegerScaleFitAtLeastLifeSize)
{
    const auto viewport = viewportFor(
        Size{.width = 200, .height = 100},
        Size{.width = 480, .height = 270},
        antwika::gfx::Fit::IntegerScale);

    EXPECT_EQ(viewport.numerator, 1U);
    EXPECT_EQ(viewport.denominator, 1U);
}

TEST(ViewportTest, ViewportFor_LandsEveryCanvasPixelOnAWholeBlock)
{
    const auto viewport = viewportFor(
        Size{.width = 1280, .height = 720},
        Size{.width = 480, .height = 270},
        antwika::gfx::Fit::IntegerScale);

    for (std::int32_t index = 0; index < 480; ++index)
    {
        const auto one = viewport.toWindow(
            Point{.x = index, .y = 0});
        const auto nextPoint = viewport.toWindow(
            Point{.x = index + 1, .y = 0});

        EXPECT_EQ(nextPoint.x - one.x, 2);
    }
}
