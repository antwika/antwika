#pragma once

#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

#include "Node.hpp"
#include "Saturate.hpp"

namespace antwika::ui::detail
{

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE marker below covers.

    /**
     * @brief Build the bar drawn where a focused caret sits.
     *
     * One glyph cell tall and one theme pixel wide, so it reads as a
     * caret at every scale and lands on the same grid the text does.
     *
     * @param theme The colours and metrics to build it from.
     * @return The node to put between the two halves of a line.
     */
    [[nodiscard]] inline Node caretNode(const Theme &theme)
    {
        const auto height = clampToU32(
            std::uint64_t{antwika::gfx::kGlyphLineHeight}
            * theme.textScale);

        const auto width = theme.textScale > 0 ? theme.textScale : 1;

        return Node{ // GCOVR_EXCL_LINE
            .width = fixedSize(width),
            .height = fixedSize(height),
            .background = theme.caret};
    }

} // namespace antwika::ui::detail
