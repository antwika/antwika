#pragma once

#include <cstdint>

namespace antwika::ui
{

    /**
     * @brief Which way a container stacks its children.
     *
     * Names the axis children advance along, so the other one is the
     * cross axis a child is aligned across.
     */
    enum class Axis : std::uint8_t
    {
        Row = 0,
        Column,
    };

} // namespace antwika::ui
