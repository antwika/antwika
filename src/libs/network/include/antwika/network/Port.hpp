#pragma once

#include <cstdint>

namespace antwika::network
{

    enum class Port : std::uint16_t
    {
    };

    [[nodiscard]] constexpr std::uint16_t rawValue(Port port) noexcept
    {
        return static_cast<std::uint16_t>(port);
    }

}
