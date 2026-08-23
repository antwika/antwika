#include "antwika/text/TextLayout.hpp"

#include <cstdint>
#include <string_view>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    gfx::Size textSize(std::string_view text, std::uint32_t scale) noexcept
    {
        const auto height = gfx::scaledGlyphLineHeight(scale);

        if (text.empty() || height == 0)
        {
            return gfx::Size{};
        }

        const auto cells = static_cast<std::uint32_t>(text.size());
        return gfx::Size{
            .width = cells * gfx::scaledGlyphAdvance(scale),
            .height = height,
        };
    }

}
