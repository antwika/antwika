#pragma once

#include <map>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/text/GlyphCells.hpp"

namespace antwika::text
{

    class GlyphCellsCache final
    {
    public:
        [[nodiscard]] const GlyphCells &at(gfx::TextScale scale);

    private:
        std::map<gfx::TextScale, GlyphCells> cells;
    };

}
