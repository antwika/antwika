#pragma once

#include <cstdint>

namespace antwika::scheduler
{

    enum class Priority : std::uint8_t
    {
    };

    inline constexpr Priority kLowPriority{0};

    inline constexpr Priority kNormalPriority{1};

    inline constexpr Priority kHighPriority{2};

    inline constexpr Priority kCriticalPriority{3};

    [[nodiscard]] constexpr std::uint8_t rawValue(Priority priority) noexcept
    {
        return static_cast<std::uint8_t>(priority);
    }

}
