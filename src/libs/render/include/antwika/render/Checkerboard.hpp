#pragma once

#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::render
{

    [[nodiscard]] gfx::Bitmap checkered(
        gfx::Size size, std::uint32_t check = 1);

}
