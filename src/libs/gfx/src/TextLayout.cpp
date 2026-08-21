#include "antwika/gfx/TextLayout.hpp"

#include <cstdint>
#include <string_view>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    Size textSize(std::string_view text, std::uint32_t scale) noexcept
    {
        const auto height = scaledGlyphLineHeight(scale);

        if (text.empty() || height == 0)
        {
            return Size{};
        }

        const auto cells = static_cast<std::uint32_t>(text.size());
        return Size{
            .width = cells * scaledGlyphAdvance(scale),
            .height = height,
        };
    }

}
