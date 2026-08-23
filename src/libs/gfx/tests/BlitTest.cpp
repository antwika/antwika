#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "antwika/gfx/Blit.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::isBlitIsInBounds;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr Size kTextureSize{.width = 8, .height = 8};

    constexpr Rect kWholeTextureRect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 8, .height = 8}};

    constexpr Rect kSomewhereOnCanvasRect{
        .originPoint = {.x = 10, .y = 10},
        .size = {.width = 16, .height = 16}};
}

TEST(BlitTest, BlitIsInBounds_AcceptsTheWholeTexture)
{
    EXPECT_TRUE(
        isBlitIsInBounds(
            kTextureSize,
            kWholeTextureRect,
            kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_AcceptsASubRectangle)
{
    EXPECT_TRUE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = 4, .y = 4},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_AcceptsADestinationPartlyOffCanvas)
{
    EXPECT_TRUE(isBlitIsInBounds(
        kTextureSize,
        kWholeTextureRect,
        Rect{
            .originPoint = {.x = -8, .y = -8},
            .size = {.width = 16, .height = 16}}));
}

TEST(BlitTest, BlitIsInBounds_RejectsAnEmptySource)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{.originPoint = {}, .size = {.width = 0, .height = 8}},
        kSomewhereOnCanvasRect));

    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{.originPoint = {}, .size = {.width = 8, .height = 0}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_RejectsAnEmptyDestination)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        kWholeTextureRect,
        Rect{.originPoint = {}, .size = {.width = 0, .height = 16}}));

    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        kWholeTextureRect,
        Rect{.originPoint = {}, .size = {.width = 16, .height = 0}}));
}

TEST(BlitTest, BlitIsInBounds_RejectsANegativeSourceOrigin)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = -1, .y = 0},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvasRect));

    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = 0, .y = -1},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_RejectsASourcePastTheRightEdge)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = 5, .y = 0},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_RejectsASourcePastTheBottomEdge)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = 0, .y = 5},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_RejectsASourceThatWouldWrapRound)
{
    constexpr auto kFarRight = std::numeric_limits<std::int32_t>::max();
    constexpr std::uint32_t kWrapping = 2147483649;

    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = kFarRight, .y = 0},
            .size = {.width = kWrapping, .height = 4}},
        kSomewhereOnCanvasRect));

    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        Rect{
            .originPoint = {.x = 0, .y = kFarRight},
            .size = {.width = 4, .height = kWrapping}},
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_TakesASourceReadTheOtherWayAbout)
{
    EXPECT_TRUE(isBlitIsInBounds(
        kTextureSize,
        antwika::gfx::RectF(
            antwika::geometry::PointF{0.0F, 0.0F},
            antwika::geometry::SizeF{8.0F, -8.0F}),
        kSomewhereOnCanvasRect));
}

TEST(BlitTest, BlitIsInBounds_RefusesOneReadPastTheTextureEitherWay)
{
    EXPECT_FALSE(isBlitIsInBounds(
        kTextureSize,
        antwika::gfx::RectF(
            antwika::geometry::PointF{0.0F, 0.0F},
            antwika::geometry::SizeF{8.0F, -9.0F}),
        kSomewhereOnCanvasRect));
}
