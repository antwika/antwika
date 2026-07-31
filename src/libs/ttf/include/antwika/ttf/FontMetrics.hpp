#pragma once

namespace antwika::ttf
{

    /**
     * @brief What a font says about the line it draws, in whole pixels
     * at one requested pixel height.
     *
     * Pixels rather than the design units the file holds, because every
     * caller wants pixels and rounding twice is how a baseline ends up
     * one row from where the glyphs were rasterised.
     * The library rounds once, here, and the whole public interface is
     * integral: no floating-point value crosses it, so nothing a caller
     * lays out can drift between two toolchains.
     */
    struct FontMetrics
    {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        int lineHeight = 0;

        /**
         * @brief Compare two sets of metrics.
         * @param other The metrics to compare against.
         * @return True when all four fields match.
         */
        [[nodiscard]] bool operator==(const FontMetrics &other) const
            = default;
    };

} // namespace antwika::ttf
