#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "antwika/font/FontMetrics.hpp"
#include "antwika/font/Glyph.hpp"
#include "antwika/font/GlyphMetrics.hpp"

namespace antwika::font
{

    namespace detail
    {
        class Rasteriser;
    } // namespace detail

    /**
     * @brief One parsed TrueType font, answering metrics and
     * rasterising glyphs.
     *
     * A font is built from bytes and never from a path: this library
     * opens no files, exactly as gfx::PngReader and sound::WavReader
     * do not, so every refusal below is reachable from a handful of
     * crafted bytes and none of them needs a fixture on disk.
     * TtfReader is the counterpart that turns a stream into one.
     *
     * **Lookup is total.** A codepoint the font has no glyph for
     * rasterises as glyph zero -- the .notdef box the font itself
     * chose -- rather than throwing, following antwika::i18n's rule
     * that anything running while a frame is drawn answers rather than
     * fails.  has() is how a caller asks in advance.
     *
     * Every size is a pixel height, and every number that comes back is
     * a whole number of pixels at that height.
     */
    class Font final
    {
    public:
        /**
         * @brief Parse a font held in memory.
         * @param bytes The whole file, moved in and kept: a rasteriser
         * reads outlines out of it for as long as this font lives.
         * @throws FontError If the bytes are too short, are a font
         * collection rather than a font, are not a font at all, carry a
         * table directory that does not fit inside them, or lack a
         * table needed to draw with.
         */
        explicit Font(std::vector<std::uint8_t> bytes);

        ~Font();

        Font(const Font &other) = delete;
        Font(Font &&other) = delete;
        Font &operator=(const Font &other) = delete;
        Font &operator=(Font &&other) = delete;

        /**
         * @brief Ask what the font says about the line it draws.
         * @param pixelHeight How tall ascender-to-descender should be.
         * @return The metrics, in whole pixels.
         * @throws FontError If pixelHeight is zero.
         */
        [[nodiscard]] FontMetrics metrics(
            std::uint32_t pixelHeight) const;

        /**
         * @brief Ask whether the font draws a codepoint itself.
         * @param codepoint The character to look for.
         * @return True when the font maps it to a glyph of its own,
         * false when it would fall back to .notdef.
         */
        [[nodiscard]] bool has(char32_t codepoint) const;

        /**
         * @brief Ask where one glyph sits relative to the pen.
         * @param codepoint The character to measure.
         * @param pixelHeight How tall ascender-to-descender should be.
         * @return The metrics, in whole pixels.
         * @throws FontError If pixelHeight is zero.
         */
        [[nodiscard]] GlyphMetrics glyphMetrics(
            char32_t codepoint, std::uint32_t pixelHeight) const;

        /**
         * @brief Ask how much closer a pair of characters sit than
         * their advances alone would put them.
         * @param left The character on the left.
         * @param right The character on the right.
         * @param pixelHeight How tall ascender-to-descender should be.
         * @return The adjustment to add to the left advance, in whole
         * pixels, and zero for a font carrying no kerning at all.
         * @throws FontError If pixelHeight is zero.
         */
        [[nodiscard]] int kerning(
            char32_t left,
            char32_t right,
            std::uint32_t pixelHeight) const;

        /**
         * @brief Draw one glyph into a coverage mask.
         * @param codepoint The character to draw.
         * @param pixelHeight How tall ascender-to-descender should be.
         * @return The mask and where it goes.
         * @throws FontError If pixelHeight is zero.
         */
        [[nodiscard]] Glyph rasterise(
            char32_t codepoint, std::uint32_t pixelHeight) const;

    private:
        // An incomplete type, as ecs::World holds its EntityManager.
        // So no public header here can name a rasteriser.
        // Declaring the destructor is what lets that work.
        // The delete lands in Font.cpp, where the pointee is complete.
        std::unique_ptr<detail::Rasteriser> rasteriser;
    };

} // namespace antwika::font
