#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "antwika/font/Coverage.hpp"
#include "antwika/font/Font.hpp"
#include "antwika/font/FontMetrics.hpp"
#include "antwika/font/GlyphMetrics.hpp"
#include "antwika/font/Rect.hpp"

namespace antwika::font
{

    struct AtlasGlyph final
    {
        char32_t codepoint = 0;
        Rect source;
        GlyphMetrics metrics;

        [[nodiscard]] bool operator==(const AtlasGlyph &other) const
            = default;
    };

    struct GlyphAtlas final
    {
        struct Options final
        {
            std::uint32_t maxWidth = 512;
            std::uint32_t padding = 1;
        };

        Coverage coverage;
        FontMetrics metrics;
        std::vector<AtlasGlyph> glyphs;

        [[nodiscard]] const AtlasGlyph *find(char32_t codepoint) const;

        [[nodiscard]] bool operator==(const GlyphAtlas &other) const
            = default;
    };

    [[nodiscard]] GlyphAtlas makeGlyphAtlas(
        const Font &font,
        std::span<const char32_t> codepoints,
        std::uint32_t pixelHeight,
        GlyphAtlas::Options options = {});

}
