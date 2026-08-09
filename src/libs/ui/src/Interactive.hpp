#pragma once

#include <antwika/gfx/Color.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Color;

    struct Interactive final
    {
        Color idle{};
        Color hovered{};
        Color pressed{};
    };

}
