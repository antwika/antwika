#pragma once

#include <cstdint>

namespace antwika::gfx
{

    // The two numbers gfx::textSize() is arithmetic over.
    // A character occupies exactly kGlyphAdvance by kGlyphLineHeight.
    // So a caller multiplies rather than asking a backend to measure.
    // And every backend draws into the same cells.
    // **They are frozen.**
    // Every layout in this tree is these two numbers times whole ones.
    // Moving one moves what a recorded click resolves against.
    // The ink inside a cell comes from a real font.
    // antwika/gfx/GlyphCells.hpp is what rasterises it.
    // None of that font's own metrics reaches here.
    // antwika/gfx/AtlasText.hpp says at length why it must not.

    /**
     * @brief Columns from one glyph's cell to the next one's.
     */
    inline constexpr std::uint32_t kGlyphAdvance = 6;

    /**
     * @brief Rows from one line's top edge to the next one's.
     */
    inline constexpr std::uint32_t kGlyphLineHeight = 8;

} // namespace antwika::gfx
