#pragma once

#include <string_view>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    [[nodiscard]] gfx::Size getTextSize(
        std::string_view text, gfx::TextScale scale) noexcept;

}
