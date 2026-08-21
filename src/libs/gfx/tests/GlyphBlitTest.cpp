#include <gtest/gtest.h>

#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/Rect.hpp"

using antwika::gfx::GlyphBlit;
using antwika::gfx::Rect;

namespace
{
    constexpr Rect kSourceRect{
        .originPoint = {.x = 1, .y = 2},
        .size = {.width = 3, .height = 4}};

    constexpr Rect kDestinationRect{
        .originPoint = {.x = 10, .y = 20},
        .size = {.width = 3, .height = 4}};
}

TEST(GlyphBlitTest, OperatorEquals_HoldsForMatchingRectangles)
{
    EXPECT_EQ(
        (GlyphBlit{
            .sourceRect = kSourceRect, .destinationRect = kDestinationRect}),
        (GlyphBlit{
            .sourceRect = kSourceRect, .destinationRect = kDestinationRect}));
}

TEST(GlyphBlitTest, OperatorEquals_FailsOnADifferentSource)
{
    EXPECT_NE(
        (GlyphBlit{
            .sourceRect = kSourceRect, .destinationRect = kDestinationRect}),
        (GlyphBlit{
            .sourceRect = kDestinationRect,
            .destinationRect = kDestinationRect}));
}

TEST(GlyphBlitTest, OperatorEquals_FailsOnADifferentDestination)
{
    EXPECT_NE(
        (GlyphBlit{
            .sourceRect = kSourceRect, .destinationRect = kDestinationRect}),
        (GlyphBlit{.sourceRect = kSourceRect, .destinationRect = kSourceRect}));
}
