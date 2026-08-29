#pragma once

#include <compare>
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

    enum class TextFace : std::uint32_t
    {
        Normal = 0,
        Small = 1,
    };

    inline constexpr std::uint32_t kSmallGlyphAdvance = 5;

    inline constexpr std::uint32_t kSmallGlyphLineHeight = 6;

    struct TextScale final
    {
        TextFace face = TextFace::Normal;

        std::uint32_t multiplier = 0;

        [[nodiscard]] bool operator==(const TextScale &other) const
            = default;

        [[nodiscard]] auto operator<=>(const TextScale &other) const
            = default;
    };

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

    [[nodiscard]] constexpr std::uint32_t getScaledGlyphAdvance(
        TextScale scale) noexcept
    {
        return glyphAdvanceOf(scale.face) * scale.multiplier;
    }

    [[nodiscard]] constexpr std::uint32_t getScaledGlyphLineHeight(
        TextScale scale) noexcept
    {
        return glyphLineHeightOf(scale.face) * scale.multiplier;
    }

}
