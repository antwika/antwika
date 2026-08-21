#pragma once

#include <cstdint>

namespace antwika::gfx
{

    enum class WindowId : std::uint64_t
    {
    };

    inline constexpr WindowId kNullWindowId{0};

    [[nodiscard]] constexpr std::uint64_t rawValue(WindowId idWindow) noexcept
    {
        return static_cast<std::uint64_t>(idWindow);
    }

}
