#pragma once

#include <cstdint>
#include <map>

#include "antwika/text/GlyphCells.hpp"

namespace antwika::text
{

    class GlyphCellsCache final
    {
    public:
        [[nodiscard]] const GlyphCells &at(std::uint32_t scale);

    private:
        std::map<std::uint32_t, GlyphCells> cells;
    };

}
