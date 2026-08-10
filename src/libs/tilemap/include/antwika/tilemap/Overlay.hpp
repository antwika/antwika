#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::tilemap
{

    enum class Overlay : std::uint8_t
    {
        None = 0,
        Bridge,
    };

    [[nodiscard]] constexpr Overlay enumBound(Overlay) noexcept
    {
        return Overlay::Bridge;
    }

    [[nodiscard]] std::string_view toString(Overlay overlay) noexcept;

}
