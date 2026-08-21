#pragma once

#include <cstdint>

namespace antwika::ui
{

    enum class DragOrigin : std::uint8_t
    {
        None = 0,

        Text,

        Track,
    };

    [[nodiscard]] constexpr DragOrigin lastEnumerator(DragOrigin) noexcept
    {
        return DragOrigin::Track;
    }

}
