#pragma once

#include "antwika/font/Coverage.hpp"
#include "antwika/font/GlyphMetrics.hpp"

namespace antwika::font
{

    struct Glyph final
    {
        GlyphMetrics metrics;
        Coverage coverage;

        [[nodiscard]] bool operator==(const Glyph &other) const
            = default;
    };

}
