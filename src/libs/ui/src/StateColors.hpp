#pragma once

#include <antwika/gfx/Color.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Color;

    struct StateColors final
    {
        Color idleColor{};
        Color hoveredColor{};
        Color pressedColor{};
    };

}
