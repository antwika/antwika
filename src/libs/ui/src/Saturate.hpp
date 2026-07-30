#pragma once

#include <cstdint>
#include <limits>

namespace antwika::ui::detail
{

    /**
     * @brief Narrow a wide value, stopping at the largest extent.
     *
     * Every pixel extent in this library is a 32-bit unsigned, so sizes
     * are accumulated as 64-bit and brought back through here.
     * A wrapped extent would place a widget somewhere absurd rather than
     * merely large, and an absurd position is much harder to recognise as
     * a mistake than a clamped one.
     *
     * @param value The value to narrow.
     * @return The value, or the largest extent when it exceeds one.
     */
    [[nodiscard]] inline std::uint32_t clampToU32(
        std::uint64_t value) noexcept
    {
        constexpr auto limit = std::numeric_limits<std::uint32_t>::max();

        return value > limit ? limit : static_cast<std::uint32_t>(value);
    }

} // namespace antwika::ui::detail
