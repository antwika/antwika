#pragma once

#include <cstdint>
#include <string_view>

#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    [[nodiscard]] gfx::Size getTextSize(
        std::string_view text, std::uint32_t scale) noexcept;

}
