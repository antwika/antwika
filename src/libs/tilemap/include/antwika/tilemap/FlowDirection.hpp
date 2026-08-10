#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::tilemap
{

    enum class FlowDirection : std::uint8_t
    {
        North = 0,
        East,
        South,
        West,
    };

    [[nodiscard]] constexpr FlowDirection enumBound(FlowDirection) noexcept
    {
        return FlowDirection::West;
    }

    [[nodiscard]] std::string_view toString(FlowDirection direction) noexcept;

}
