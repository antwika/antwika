#include <gtest/gtest.h>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/TextLayout.hpp"

using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::Size;
using antwika::gfx::textSize;

TEST(TextLayoutTest, TextSize_IsOneCellPerCharacter)
{
    EXPECT_EQ(
        textSize("As", 1),
        (Size{.width = 2 * kGlyphAdvance, .height = kGlyphLineHeight}));
}

TEST(TextLayoutTest, TextSize_GrowsWithTheScale)
{
    EXPECT_EQ(
        textSize("As", 3),
        (Size{
            .width = 2 * kGlyphAdvance * 3,
            .height = kGlyphLineHeight * 3}));
}

TEST(TextLayoutTest, TextSize_IsZeroForEmptyText)
{
    EXPECT_EQ(textSize("", 4), Size{});
}

TEST(TextLayoutTest, TextSize_IsZeroAtAZeroScale)
{
    EXPECT_EQ(textSize("As", 0), Size{});
}
