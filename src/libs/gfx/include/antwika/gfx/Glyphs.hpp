#pragma once

#include <cstdint>

namespace antwika::gfx
{

    /**
     * @brief Lit columns in one glyph.
     */
    inline constexpr std::uint32_t kGlyphWidth = 5;

    /**
     * @brief Lit rows in one glyph.
     */
    inline constexpr std::uint32_t kGlyphHeight = 7;

    /**
     * @brief Columns from one glyph's left edge to the next one's.
     *
     * One wider than the glyph, so adjacent characters do not touch.
     */
    inline constexpr std::uint32_t kGlyphAdvance = kGlyphWidth + 1;

    /**
     * @brief Rows from one line's top edge to the next one's.
     */
    inline constexpr std::uint32_t kGlyphLineHeight = kGlyphHeight + 1;

    /**
     * @brief Get one row of a character's glyph as a bit mask.
     *
     * The font is fixed-cell on purpose: every character occupies exactly
     * kGlyphAdvance by kGlyphLineHeight, so a caller can lay text out with
     * arithmetic instead of asking a backend to measure it, and every
     * backend draws the same picture.
     * @param character The character to look up.
     * @param row The row to read, counting down from the glyph's top.
     * @return The low kGlyphWidth bits, most significant bit leftmost;
     * zero for a space, for a row at or past kGlyphHeight, and for any
     * character the font has no glyph for.
     */
    [[nodiscard]] std::uint8_t glyphRow(
        char character, std::uint32_t row) noexcept;

} // namespace antwika::gfx
