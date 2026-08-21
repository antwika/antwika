#pragma once

#include <cstdint>
#include <map>

#include "antwika/gfx/GlyphCells.hpp"

namespace antwika::gfx
{

    class GlyphCellsCache final
    {
    public:
        [[nodiscard]] const GlyphCells &at(std::uint32_t scale);

    private:
        std::map<std::uint32_t, GlyphCells> cells;
    };

}
