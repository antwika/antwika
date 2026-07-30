#pragma once

#include <antwika/gfx/Color.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Color;

    /**
     * @brief The colours a resolved ButtonState picks between.
     *
     * Taken from the theme when the widget is declared, so resolving an
     * appearance needs no Theme and stays a function of the arena and the
     * pointer alone.
     */
    struct Interactive
    {
        Color idle{};
        Color hovered{};
        Color pressed{};
    };

} // namespace antwika::ui::detail
