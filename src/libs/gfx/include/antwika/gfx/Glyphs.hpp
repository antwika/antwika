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

}
