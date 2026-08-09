#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "antwika/gfx/Blit.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::blitIsDrawable;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr Size kTexture{.width = 8, .height = 8};

    constexpr Rect kWholeTexture{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 8, .height = 8}};

    constexpr Rect kSomewhereOnCanvas{
        .origin = {.x = 10, .y = 10},
        .size = {.width = 16, .height = 16}};
}

TEST(BlitTest, BlitIsDrawable_AcceptsTheWholeTexture)
{
    EXPECT_TRUE(
        blitIsDrawable(kTexture, kWholeTexture, kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_AcceptsASubRectangle)
{
    EXPECT_TRUE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = 4, .y = 4},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_AcceptsADestinationPartlyOffCanvas)
{
    EXPECT_TRUE(blitIsDrawable(
        kTexture,
        kWholeTexture,
        Rect{
            .origin = {.x = -8, .y = -8},
            .size = {.width = 16, .height = 16}}));
}

TEST(BlitTest, BlitIsDrawable_RejectsAnEmptySource)
{
    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{.origin = {}, .size = {.width = 0, .height = 8}},
        kSomewhereOnCanvas));

    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{.origin = {}, .size = {.width = 8, .height = 0}},
        kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_RejectsAnEmptyDestination)
{
    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        kWholeTexture,
        Rect{.origin = {}, .size = {.width = 0, .height = 16}}));

    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        kWholeTexture,
        Rect{.origin = {}, .size = {.width = 16, .height = 0}}));
}

TEST(BlitTest, BlitIsDrawable_RejectsANegativeSourceOrigin)
{
    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = -1, .y = 0},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvas));

    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = 0, .y = -1},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_RejectsASourcePastTheRightEdge)
{
    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = 5, .y = 0},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_RejectsASourcePastTheBottomEdge)
{
    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = 0, .y = 5},
            .size = {.width = 4, .height = 4}},
        kSomewhereOnCanvas));
}

TEST(BlitTest, BlitIsDrawable_RejectsASourceThatWouldWrapRound)
{
    constexpr auto kFarRight = std::numeric_limits<std::int32_t>::max();
    constexpr std::uint32_t kWrapping = 2147483649;

    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = kFarRight, .y = 0},
            .size = {.width = kWrapping, .height = 4}},
        kSomewhereOnCanvas));

    EXPECT_FALSE(blitIsDrawable(
        kTexture,
        Rect{
            .origin = {.x = 0, .y = kFarRight},
            .size = {.width = 4, .height = kWrapping}},
        kSomewhereOnCanvas));
}
