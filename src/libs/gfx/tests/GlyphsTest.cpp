#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/Glyphs.hpp"

using antwika::gfx::glyphRow;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphHeight;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::kGlyphWidth;

namespace
{
    constexpr char kFirstPrintable = ' ';
    constexpr char kLastPrintable = '~';

    // A glyph row only ever describes columns the cell actually has.
    constexpr std::uint8_t kRowMask =
        static_cast<std::uint8_t>((1U << kGlyphWidth) - 1U);
} // namespace

TEST(GlyphsTest, Metrics_LeaveAGapAfterEveryCell)
{
    EXPECT_EQ(kGlyphAdvance, kGlyphWidth + 1);
    EXPECT_EQ(kGlyphLineHeight, kGlyphHeight + 1);
}

TEST(GlyphsTest, GlyphRow_StaysWithinTheCellForEveryPrintableCharacter)
{
    for (char character = kFirstPrintable; character <= kLastPrintable;
         ++character)
    {
        for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
        {
            EXPECT_EQ(glyphRow(character, row) & ~kRowMask, 0)
                << "character " << character << " row " << row;
        }
    }
}

TEST(GlyphsTest, GlyphRow_DrawsSomethingForEveryPrintableNonSpace)
{
    for (char character = kFirstPrintable + 1; character <= kLastPrintable;
         ++character)
    {
        std::uint8_t lit = 0;
        for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
        {
            lit |= glyphRow(character, row);
        }

        EXPECT_NE(lit, 0) << "character " << character;
    }
}

TEST(GlyphsTest, GlyphRow_ReadsTheRowsOfAKnownGlyphTopToBottom)
{
    // 'T' is a bar across the top with a stem down the middle.
    EXPECT_EQ(glyphRow('T', 0), 0b11111);
    for (std::uint32_t row = 1; row < kGlyphHeight; ++row)
    {
        EXPECT_EQ(glyphRow('T', row), 0b00100) << "row " << row;
    }
}

TEST(GlyphsTest, GlyphRow_DrawsNothingForASpace)
{
    for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
    {
        EXPECT_EQ(glyphRow(' ', row), 0) << "row " << row;
    }
}

TEST(GlyphsTest, GlyphRow_DrawsNothingBelowThePrintableRange)
{
    EXPECT_EQ(glyphRow('\n', 0), 0);
}

TEST(GlyphsTest, GlyphRow_DrawsNothingAboveThePrintableRange)
{
    EXPECT_EQ(glyphRow('\x7f', 0), 0);
    EXPECT_EQ(glyphRow('\xc3', 0), 0);
}

TEST(GlyphsTest, GlyphRow_DrawsNothingPastTheLastRow)
{
    EXPECT_EQ(glyphRow('T', kGlyphHeight), 0);
    EXPECT_EQ(glyphRow('T', kGlyphHeight + 1), 0);
}
