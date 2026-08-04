#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief The built-in font, drawn once into the fixed cells
     * gfx::textSize() measures, at one scale.
     *
     * A cell is kGlyphAdvance by kGlyphLineHeight glyph pixels, times
     * the scale, and holds one character's coverage: how much ink
     * landed on each pixel of it, 0 to 255.
     * The glyphs come from a real font rather than from a table of
     * bits, so the cells are rasterised rather than written down -- and
     * the whole of that work happens here, once per scale, rather than
     * per line of text or per frame.
     *
     * **The cell grid is the point of this type, and it is the reason
     * nothing gfx::textSize() reports moves.** The pen still steps one
     * fixed cell per character and a line is still kGlyphLineHeight
     * tall, so every layout and every hit test in the tree measures
     * exactly what it measured before; what changed is the ink inside a
     * cell.  Nothing here is ever asked how wide a string is, and
     * nothing here may be: the font's own metrics are scaled by a float
     * taken from an outline, and a last-bit difference between two
     * toolchains costs a pixel where it is drawn and would cost a
     * divergent replay where a click is resolved against it.
     * antwika/gfx/AtlasText.hpp writes that argument out in full.
     *
     * Ink is clipped to the cell rather than trusted to fit it, so a
     * character can never light a pixel the box textSize() reports does
     * not contain -- whatever the font says about a glyph.
     *
     * A plain value with no resource in it, so cells may be built,
     * asserted on and thrown away with no window anywhere in sight.
     */
    class GlyphCells final
    {
    public:
        /**
         * @brief Rasterise every character the built-in font covers.
         * @param scale Pixels per glyph pixel; zero leaves cells of no
         * size, holding no coverage at all, which is what a zero scale
         * draws.
         */
        explicit GlyphCells(std::uint32_t scale);

        /**
         * @brief Ask how big one character's cell is.
         * @return kGlyphAdvance by kGlyphLineHeight, times the scale.
         */
        [[nodiscard]] Size cellSize() const noexcept;

        /**
         * @brief Read how much ink one pixel of one character holds.
         *
         * Total, like every other lookup on the drawing path: a
         * character outside the range the font covers, and a position
         * outside the cell, are both blank rather than errors.
         * @param character The character to read.
         * @param column The column, from the cell's left edge.
         * @param row The row, from the cell's top edge.
         * @return The coverage, 0 for a pixel with no ink on it.
         */
        [[nodiscard]] std::uint8_t coverageAt(
            char character,
            std::uint32_t column,
            std::uint32_t row) const noexcept;

    private:
        Size cell;
        std::vector<std::uint8_t> samples;
    };

    /**
     * @brief The cells for every scale drawn at so far, built the
     * first time a line of text is drawn at each and then kept.
     *
     * Text is drawn every frame and rasterising a font is not frame
     * work, so the cells for a scale are built once and kept.
     * What is kept is a memo of a pure function and nothing else: the
     * cells for a scale are decided by the scale and by bytes the build
     * compiled in, so a cache changes how long a call takes and can
     * change nothing else.
     * A value the caller owns rather than a static, so who can reach a
     * cache and how long it lives are its owner's to decide: a renderer
     * keeps one to itself for its own lifetime, and no state outlives
     * every owner.  What the cells *hold* stays this library's answer
     * either way -- wiki/libraries/gfx.md says why that is the line
     * that matters.
     */
    class GlyphCellsCache final
    {
    public:
        /**
         * @brief Get the cells for one scale, building them if this is
         * the first ask at it.
         * @param scale Pixels per glyph pixel.
         * @return The cells, which live as long as this cache does and
         * stay put as later scales arrive.
         */
        [[nodiscard]] const GlyphCells &at(std::uint32_t scale);

    private:
        // Keyed by scale, and a map rather than a vector on purpose.
        // A reference handed out here outlives every later arrival.
        std::map<std::uint32_t, GlyphCells> cells;
    };

} // namespace antwika::gfx
