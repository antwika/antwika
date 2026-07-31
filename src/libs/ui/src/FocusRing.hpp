#pragma once

#include <cstdint>

#include <antwika/gfx/Color.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Color;

    /**
     * @brief The border a focused widget draws around itself.
     *
     * Taken from the theme when the widget is declared, the way
     * Interactive is, so resolving focus needs no Theme and stays a
     * function of the arena and the frame's input alone.
     *
     * antwika::gfx has no stroke primitive, so this is drawn as four
     * filled bars and the thickness is what they are.
     */
    struct FocusRing
    {
        Color color{};
        std::uint32_t thickness = 0;
    };

} // namespace antwika::ui::detail
