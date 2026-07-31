#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace antwika::ttf::tests
{

    /**
     * @brief A whole, valid, drawable TrueType font, built in memory.
     *
     * **Nothing is checked into this repository to test this library
     * with**, which is deliberate and is what the byte-stream entry
     * point buys: a font binary would be a licence to track, a
     * megabyte in every clone, and a fixture nobody could read a diff
     * of.  It would also leave every refusal below untestable, since a
     * real font is by definition not malformed in any of the ways they
     * catch.
     *
     * The font holds four glyphs: .notdef and a space, both empty, and
     * two filled rectangles at U+0041 and U+0042.  Every measurement
     * is a round number at 1000 units per em -- the ascender is 800,
     * the descender -200 -- so a test asserts on exact pixels rather
     * than on a tolerance, and a change in rounding is visible instead
     * of absorbed.
     */
    struct FontRecipe
    {
        /**
         * @brief The four bytes the file opens with.
         */
        std::uint32_t flavour = 0x00010000;

        /**
         * @brief Whether to write the 'kern' table at all.
         */
        bool kerning = true;
    };

    /**
     * @brief Build the font a recipe describes.
     * @param recipe What to vary from the standard font.
     * @return The whole file.
     */
    [[nodiscard]] std::vector<std::uint8_t> buildFont(
        FontRecipe recipe = {});

    /**
     * @brief One entry of a table directory, written verbatim.
     */
    struct TableRecord
    {
        std::string_view tag;
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

    /**
     * @brief Build an offset table and nothing else, for the refusals
     * that never reach a rasteriser.
     * @param flavour The four bytes the file opens with.
     * @param declaredTables The count to write into the header, which
     * need not be how many records follow.
     * @param records The records to write.
     * @param totalSize How long to make the whole blob, padding with
     * zeroes.
     * @return The blob.
     */
    [[nodiscard]] std::vector<std::uint8_t> buildDirectory(
        std::uint32_t flavour,
        std::uint16_t declaredTables,
        std::span<const TableRecord> records,
        std::size_t totalSize);

} // namespace antwika::ttf::tests
