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

    /**
     * @brief One glyph's place in an atlas, and where it goes relative
     * to the pen once it is drawn from there.
     */
    struct AtlasGlyph
    {
        char32_t codepoint = 0;
        Rect source;
        GlyphMetrics metrics;

        /**
         * @brief Compare two entries.
         * @param other The entry to compare against.
         * @return True when all three fields match.
         */
        [[nodiscard]] bool operator==(const AtlasGlyph &other) const
            = default;
    };

    /**
     * @brief A set of glyphs packed into one coverage mask, with the
     * map from a character to the rectangle holding it.
     *
     * This is the shape a renderer wants: one mask becomes one texture,
     * and drawing a string is then one blit per character out of a
     * source rectangle, rather than a texture per glyph.
     * It is a plain value with no resource in it, so an atlas may be
     * built, asserted on and thrown away with no window anywhere in
     * sight -- which is what lets the whole of this library be tested
     * headless.
     *
     * Entries are sorted by codepoint, which is what makes find() a
     * binary search and makes two atlases over the same characters
     * compare equal whatever order they were asked for in.
     */
    struct GlyphAtlas
    {
        /**
         * @brief What to pack into, rather than what to pack.
         */
        struct Options
        {
            std::uint32_t maxWidth = 512;
            std::uint32_t padding = 1;
        };

        Coverage coverage;
        FontMetrics metrics;
        std::vector<AtlasGlyph> glyphs;

        /**
         * @brief Look one character up.
         * @param codepoint The character to find.
         * @return Its entry, or nullptr when this atlas does not hold
         * it.  The pointer is into this atlas and dies with it.
         */
        [[nodiscard]] const AtlasGlyph *find(char32_t codepoint) const;

        /**
         * @brief Compare two atlases.
         * @param other The atlas to compare against.
         * @return True when the masks, the metrics and the entries all
         * match.
         */
        [[nodiscard]] bool operator==(const GlyphAtlas &other) const
            = default;
    };

    /**
     * @brief Rasterise a set of characters and pack them into one mask.
     *
     * Packing is a shelf: glyphs go left to right in ascending
     * codepoint order, and a glyph that will not fit the row starts the
     * next one.  That is deliberately not the tightest packing there
     * is -- it is the one whose result depends on nothing but the
     * arguments, so the same request builds the same atlas byte for
     * byte on every machine.
     *
     * @param font The font to draw from.
     * @param codepoints The characters to include; duplicates collapse
     * and order does not matter.
     * @param pixelHeight How tall ascender-to-descender should be.
     * @param options What to pack into.
     * @return The packed atlas.
     * @throws FontError If no characters were asked for, if pixelHeight
     * is zero, or if a glyph is too wide to fit a row of its own.
     */
    [[nodiscard]] GlyphAtlas makeGlyphAtlas(
        const Font &font,
        std::span<const char32_t> codepoints,
        std::uint32_t pixelHeight,
        GlyphAtlas::Options options = {});

} // namespace antwika::font
