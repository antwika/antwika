#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    class GlyphCells final
    {
    public:
        explicit GlyphCells(std::uint32_t scale);

        [[nodiscard]] Size cellSize() const noexcept;

        [[nodiscard]] std::uint8_t coverageAt(
            char character,
            std::uint32_t column,
            std::uint32_t row) const noexcept;

    private:
        Size cell;
        std::vector<std::uint8_t> samples;
    };

    class GlyphCellsCache final
    {
    public:
        [[nodiscard]] const GlyphCells &at(std::uint32_t scale);

    private:
        std::map<std::uint32_t, GlyphCells> cells;
    };

}
