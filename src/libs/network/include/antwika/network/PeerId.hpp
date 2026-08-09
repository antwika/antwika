#pragma once

#include <cstdint>

namespace antwika::network
{

    enum class PeerId : std::uint32_t
    {
    };

    [[nodiscard]] constexpr std::uint32_t rawValue(PeerId peer) noexcept
    {
        return static_cast<std::uint32_t>(peer);
    }

}
