#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::holdem
{

    enum class SeatId : std::uint8_t
    {
    };

    [[nodiscard]] constexpr std::uint8_t rawValue(SeatId seat) noexcept
    {
        return static_cast<std::uint8_t>(seat);
    }

    [[nodiscard]] constexpr std::size_t indexOf(SeatId seat) noexcept
    {
        return static_cast<std::size_t>(rawValue(seat));
    }

    [[nodiscard]] constexpr SeatId makeSeatId(std::size_t index) noexcept
    {
        return static_cast<SeatId>(static_cast<std::uint8_t>(index));
    }

}
