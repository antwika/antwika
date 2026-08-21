#pragma once

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
        Rect sourceRect;
        GlyphMetrics metrics;

        [[nodiscard]] bool operator==(const AtlasGlyph &other) const
            = default;
    };

}
