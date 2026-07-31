#pragma once

#include "antwika/ttf/Coverage.hpp"
#include "antwika/ttf/GlyphMetrics.hpp"

namespace antwika::ttf
{

    /**
     * @brief One rasterised glyph: where it goes and what it covers.
     *
     * A glyph with nothing to draw -- a space, or a codepoint whose
     * .notdef is empty -- is an ordinary Glyph with an empty coverage
     * and a non-zero advance, never an error.
     */
    struct Glyph
    {
        GlyphMetrics metrics;
        Coverage coverage;

        /**
         * @brief Compare two glyphs.
         * @param other The glyph to compare against.
         * @return True when the metrics match and the coverage does.
         */
        [[nodiscard]] bool operator==(const Glyph &other) const
            = default;
    };

} // namespace antwika::ttf
