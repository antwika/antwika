#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Color;

    struct FocusRing final
    {
        Color color{};
        std::uint32_t thickness = 0;
    };

}
