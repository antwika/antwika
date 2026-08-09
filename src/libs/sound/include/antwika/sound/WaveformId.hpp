#pragma once

#include <cstdint>

namespace antwika::sound
{

    enum class WaveformId : std::uint32_t
    {
    };

    [[nodiscard]] constexpr std::uint32_t rawValue(WaveformId id) noexcept
    {
        return static_cast<std::uint32_t>(id);
    }

}
