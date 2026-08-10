#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::gfx
{

    inline constexpr std::uint32_t kGlyphAdvance = 6;

    inline constexpr std::uint32_t kGlyphLineHeight = 8;

    inline constexpr char32_t kFirstGlyph = U' ';

    inline constexpr char32_t kLastGlyph = U'~';

    inline constexpr std::size_t kGlyphCount =
        kLastGlyph - kFirstGlyph + 1;

    /**
     * @brief Selects which rasterization of the built-in font
     *        a text scale refers to.
     *
     * Normal is the original 8-pixel line height face and Small is
     * the same font rasterized at a 6-pixel line height.
     */
    enum class TextFace : std::uint32_t
    {
        Normal = 0,
        Small = 1,
    };

    inline constexpr std::uint32_t kSmallGlyphAdvance = 5;

    inline constexpr std::uint32_t kSmallGlyphLineHeight = 6;

    inline constexpr std::uint32_t kTextFaceShift = 24;

    inline constexpr std::uint32_t kTextMultiplierMask =
        (std::uint32_t{1} << kTextFaceShift) - 1;

    /**
     * @brief Packs a face and a multiplier into one text scale.
     *
     * Ensures: the Normal face encodes to the bare multiplier, so
     *          every pre-existing scale value keeps its meaning.
     */
    [[nodiscard]] constexpr std::uint32_t encodeTextScale(
        TextFace face, std::uint32_t multiplier) noexcept
    {
        return (static_cast<std::uint32_t>(face) << kTextFaceShift)
               | (multiplier & kTextMultiplierMask);
    }

    [[nodiscard]] constexpr TextFace textFaceOf(
        std::uint32_t scale) noexcept
    {
        return static_cast<TextFace>(scale >> kTextFaceShift);
    }

    [[nodiscard]] constexpr std::uint32_t textMultiplierOf(
        std::uint32_t scale) noexcept
    {
        return scale & kTextMultiplierMask;
    }

    [[nodiscard]] constexpr std::uint32_t glyphAdvanceOf(
        TextFace face) noexcept
    {
        return face == TextFace::Small ? kSmallGlyphAdvance
                                       : kGlyphAdvance;
    }

    [[nodiscard]] constexpr std::uint32_t glyphLineHeightOf(
        TextFace face) noexcept
    {
        return face == TextFace::Small ? kSmallGlyphLineHeight
                                       : kGlyphLineHeight;
    }

    [[nodiscard]] constexpr std::uint32_t scaledGlyphAdvance(
        std::uint32_t scale) noexcept
    {
        return glyphAdvanceOf(textFaceOf(scale))
               * textMultiplierOf(scale);
    }

    [[nodiscard]] constexpr std::uint32_t scaledGlyphLineHeight(
        std::uint32_t scale) noexcept
    {
        return glyphLineHeightOf(textFaceOf(scale))
               * textMultiplierOf(scale);
    }

}
