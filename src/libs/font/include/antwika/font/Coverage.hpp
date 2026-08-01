#pragma once

#include <cstdint>
#include <vector>

namespace antwika::font
{

    /**
     * @brief An 8-bit coverage mask: how much of each pixel a glyph
     * covered, laid out row by row from the top-left with no padding.
     *
     * **This is antwika::font's own value type rather than
     * antwika::gfx::Bitmap, and that is the library's central
     * decision.**
     * A glyph has one channel, not four: it says how much ink landed on
     * a pixel and says nothing about colour, which is the tint's answer
     * at the moment it is drawn.
     * Storing it as RGBA would quadruple an atlas and would bake a
     * colour into a mask that has none.
     * Reusing gfx::Bitmap would also make antwika::font depend on
     * antwika::gfx -- and through it on GLM and antwika::log -- for one
     * struct of three fields, where the library as it stands depends on
     * nothing at all, exactly as antwika::animation and antwika::wfc do.
     * Expanding a mask into straight RGBA is four lines at the one seam
     * that wants a texture, and wiki/libraries/font.md writes them out.
     *
     * Deliberately a plain value: nothing here owns a resource and
     * nothing here has a lifetime rule, so a mask may be copied, stored
     * and compared freely.
     */
    struct Coverage
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> samples;

        /**
         * @brief Check that this mask holds exactly the samples it
         * claims to.
         * @return True when samples holds width * height entries.
         */
        [[nodiscard]] bool isComplete() const noexcept;

        /**
         * @brief Read one sample.
         * @param x The column, from the left.
         * @param y The row, from the top.
         * @return How much of that pixel the glyph covered, 0 to 255.
         * @throws FontError If the position lies outside the mask.
         */
        [[nodiscard]] std::uint8_t at(
            std::uint32_t x, std::uint32_t y) const;

        /**
         * @brief Compare two masks.
         * @param other The mask to compare against.
         * @return True when the sizes match and every sample does.
         */
        [[nodiscard]] bool operator==(const Coverage &other) const
            = default;
    };

} // namespace antwika::font
