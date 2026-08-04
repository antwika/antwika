#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace antwika::font::detail
{

    /**
     * @brief A font the rasteriser has been pointed at, and the bytes
     * it is pointed into.
     *
     * Incomplete here on purpose, so stb_truetype's own types reach no
     * header of ours and nothing outside StbTrueType.cpp can name the
     * rasteriser this library was built on.
     * The two are one object because the rasteriser keeps a bare
     * pointer into the font bytes for as long as it is used, and a
     * font whose bytes and whose reader can be separated is one
     * somebody will separate.
     */
    class FontData;

    /**
     * @brief What a font says about the line it draws, in the design
     * units the file holds.
     */
    struct VerticalMetrics
    {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
    };

    /**
     * @brief The pixels a glyph lands on, relative to the pen on the
     * baseline, with y growing downwards.
     */
    struct GlyphBox
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    /**
     * @brief One drawn glyph, as the rasteriser handed it over.
     */
    struct RasterisedGlyph
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> samples;
    };

    /**
     * @brief The whole of this library's contact with stb_truetype.
     *
     * Everything below is a one-call wrapper, and deliberately so:
     * StbTrueType.cpp is the one translation unit that compiles the
     * rasteriser, so it is also the one compiled with warnings off,
     * and code with a decision in it does not belong somewhere the
     * compiler has been told to stay quiet.
     * Rounding, refusals and packing therefore all live in Font.cpp
     * and GlyphAtlas.cpp, which are checked like any other source.
     */
    class Rasteriser final
    {
    public:
        /**
         * @brief Point the rasteriser at a font held in memory.
         * @param bytes The whole file, moved in and kept.
         * @throws FontError If the rasteriser will not read it.
         */
        explicit Rasteriser(std::vector<std::uint8_t> bytes);

        ~Rasteriser();

        Rasteriser(const Rasteriser &other) = delete;
        Rasteriser(Rasteriser &&other) = delete;
        Rasteriser &operator=(const Rasteriser &other) = delete;
        Rasteriser &operator=(Rasteriser &&other) = delete;

        /**
         * @brief Read the font's vertical metrics.
         * @return Its metrics, in design units.
         */
        [[nodiscard]] VerticalMetrics verticalMetrics() const;

        /**
         * @brief Work out what design units scale by to reach a pixel
         * height.
         * @param pixelHeight How tall ascender-to-descender should be.
         * @return The factor to multiply a design unit by.
         */
        [[nodiscard]] float scaleForPixelHeight(
            std::uint32_t pixelHeight) const;

        /**
         * @brief Map a character to the glyph the font draws for it.
         * @param codepoint The character to map.
         * @return The glyph index, or zero for .notdef.
         */
        [[nodiscard]] int glyphIndex(char32_t codepoint) const;

        /**
         * @brief Read how far the pen moves past one glyph.
         * @param glyph The glyph index.
         * @return The advance, in design units.
         */
        [[nodiscard]] int advanceWidth(int glyph) const;

        /**
         * @brief Work out where a glyph lands without drawing it.
         * @param glyph The glyph index.
         * @param scale The factor from scaleForPixelHeight().
         * @return The box, all zeroes for a glyph with nothing to draw.
         */
        [[nodiscard]] GlyphBox glyphBox(int glyph, float scale) const;

        /**
         * @brief Draw one glyph.
         * @param glyph The glyph index.
         * @param scale The factor from scaleForPixelHeight().
         * @return The coverage samples, empty for a glyph with nothing
         * to draw.
         */
        [[nodiscard]] RasterisedGlyph rasteriseGlyph(
            int glyph, float scale) const;

    private:
        std::unique_ptr<FontData> font;
    };

} // namespace antwika::font::detail
