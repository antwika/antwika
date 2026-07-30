#pragma once

#include <cstdint>

namespace antwika::ui
{

    /**
     * @brief Where a child sits on its container's cross axis.
     *
     * Cross axis only, deliberately.
     * Main-axis placement is expressed with a growing spacer instead, so
     * that leading, trailing and centred content all come out of the
     * space distribution the layout already does rather than out of a
     * second rule that could disagree with it.
     */
    enum class Alignment : std::uint8_t
    {
        Start = 0,
        Center,
        End,
    };

} // namespace antwika::ui
