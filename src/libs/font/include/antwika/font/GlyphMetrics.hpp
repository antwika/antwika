#pragma once

namespace antwika::font
{

    /**
     * @brief Where one glyph sits relative to the pen, in whole pixels
     * at one requested pixel height.
     *
     * The pen walks along the baseline, and y grows downwards, so
     * bearingY is negative for everything that rises above it.
     */
    struct GlyphMetrics
    {
        int advance = 0;
        int bearingX = 0;
        int bearingY = 0;

        /**
         * @brief Compare two sets of metrics.
         * @param other The metrics to compare against.
         * @return True when all three fields match.
         */
        [[nodiscard]] bool operator==(const GlyphMetrics &other) const
            = default;
    };

} // namespace antwika::font
