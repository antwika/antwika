#pragma once

#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

#include "Node.hpp"
#include "Saturate.hpp"

namespace antwika::ui::detail
{

    [[nodiscard]] inline Node getCaretNode(const Theme &theme)
    {
        const auto height = getClampToU32(
            std::uint64_t{antwika::gfx::glyphLineHeightOf(theme.face)}
            * theme.textScale);

        const auto width = theme.textScale > 0 ? theme.textScale : 1;

        return Node{ // GCOVR_EXCL_LINE
            .widthSizing = getFixedSize(0),
            .heightSizing = getFixedSize(height),
            .backgroundColor = theme.caretColor,
            .extraWidth = width};
    }

}
