#pragma once

#include <cstdint>
#include <limits>

namespace antwika::ui::detail
{

    [[nodiscard]] inline std::uint32_t getClampToU32(
        std::uint64_t value) noexcept
    {
        constexpr auto limit = std::numeric_limits<std::uint32_t>::max();

        return value > limit ? limit : static_cast<std::uint32_t>(value);
    }

}
