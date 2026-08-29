#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    class GlyphCells final
    {
    public:
        explicit GlyphCells(gfx::TextScale scale);

        [[nodiscard]] gfx::Size getCellSize() const noexcept;

        [[nodiscard]] std::uint8_t coverageAt(
            char character,
            std::uint32_t column,
            std::uint32_t row) const noexcept;

    private:
        gfx::Size cell;
        std::vector<std::uint8_t> samples;
    };

}
