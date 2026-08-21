#pragma once

#include <cstdint>

namespace antwika::sound
{

    enum class WaveformId : std::uint32_t
    {
    };

    [[nodiscard]] constexpr std::uint32_t rawValue(
        WaveformId idWaveform) noexcept
    {
        return static_cast<std::uint32_t>(idWaveform);
    }

}
