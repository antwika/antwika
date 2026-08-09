#include <gtest/gtest.h>

#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/Rect.hpp"

using antwika::gfx::GlyphBlit;
using antwika::gfx::Rect;

namespace
{
    constexpr Rect kSource{
        .origin = {.x = 1, .y = 2},
        .size = {.width = 3, .height = 4}};

    constexpr Rect kDestination{
        .origin = {.x = 10, .y = 20},
        .size = {.width = 3, .height = 4}};
}

TEST(GlyphBlitTest, OperatorEquals_HoldsForMatchingRectangles)
{
    EXPECT_EQ(
        (GlyphBlit{.source = kSource, .destination = kDestination}),
        (GlyphBlit{.source = kSource, .destination = kDestination}));
}

TEST(GlyphBlitTest, OperatorEquals_FailsOnADifferentSource)
{
    EXPECT_NE(
        (GlyphBlit{.source = kSource, .destination = kDestination}),
        (GlyphBlit{
            .source = kDestination, .destination = kDestination}));
}

TEST(GlyphBlitTest, OperatorEquals_FailsOnADifferentDestination)
{
    EXPECT_NE(
        (GlyphBlit{.source = kSource, .destination = kDestination}),
        (GlyphBlit{.source = kSource, .destination = kSource}));
}
