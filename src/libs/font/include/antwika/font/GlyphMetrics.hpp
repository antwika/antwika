#pragma once

namespace antwika::font
{

    struct GlyphMetrics final
    {
        int advance = 0;
        int bearingX = 0;
        int bearingY = 0;

        [[nodiscard]] bool operator==(const GlyphMetrics &other) const
            = default;
    };

}
