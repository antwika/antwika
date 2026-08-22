#pragma once

#include <cstdint>

namespace antwika::ecs
{

    enum class PhaseId : std::uint32_t
    {
    };

    [[nodiscard]] constexpr std::uint32_t rawValue(PhaseId phase) noexcept
    {
        return static_cast<std::uint32_t>(phase);
    }

}
